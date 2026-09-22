#include "layout.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdarg.h>

enum {
    MAX_ACCTS = 80,
    MAX_MBRS = 32,
    MAX_TXNS = 512,
    MAX_FUNDS = 16,
    MAX_POS = 64,
    PAGE_CAP = 131072
};

static char PAGE[PAGE_CAP];
static int page_len = 0;

static void page_clear(void) {
    PAGE[0] = 0;
    page_len = 0;
}

static void page_add(const char* s) {
    int n;
    if (!s) return;
    n = (int)strlen(s);
    if (page_len + n >= PAGE_CAP) return;
    memcpy(PAGE + page_len, s, n);
    page_len += n;
    PAGE[page_len] = 0;
}

static void page_addf(const char* fmt, ...) {
    char tmp[4096];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(tmp, fmt, ap);
    va_end(ap);
    page_add(tmp);
}

static void page_esc(const char* s) {
    char one[2];
    if (!s) return;
    one[1] = 0;
    for (; *s; s++) {
        if (*s == '&') page_add("&amp;");
        else if (*s == '<') page_add("&lt;");
        else if (*s == '>') page_add("&gt;");
        else if (*s == '"') page_add("&quot;");
        else {
            one[0] = *s;
            page_add(one);
        }
    }
}

static void send_all(int fd, const char* buf, int n) {
    int off = 0;
    while (off < n) {
        int w = (int)send(fd, buf + off, n - off, 0);
        if (w <= 0) return;
        off += w;
    }
}

static void respond(int fd, int code, const char* status, const char* ctype,
                    const char* extra, const char* body, int blen) {
    char hdr[1024];
    int n;
    if (!extra) extra = "";
    n = sprintf(hdr,
                "HTTP/1.0 %d %s\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n"
                "%s"
                "\r\n",
                code, status, ctype, blen, extra);
    send_all(fd, hdr, n);
    if (body && blen > 0) send_all(fd, body, blen);
}

static void redirect_to(int fd, const char* loc, const char* cookie) {
    char extra[256];
    char body[256];
    int blen;
    extra[0] = 0;
    if (cookie && cookie[0]) {
        sprintf(extra, "Location: %s\r\nSet-Cookie: %s\r\n", loc, cookie);
    } else {
        sprintf(extra, "Location: %s\r\n", loc);
    }
    blen = sprintf(body, "<html><body><a href=\"%s\">Continue</a></body></html>", loc);
    respond(fd, 302, "Found", "text/html", extra, body, blen);
}

static void page_open(const char* title) {
    page_clear();
    page_add("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
             "\"http://www.w3.org/TR/html4/loose.dtd\">\n");
    page_add("<html><head><title>");
    page_esc(title);
    page_add("</title></head>\n");
    page_add("<body bgcolor=\"#F4F0E4\" text=\"#1A1A1A\" link=\"#000080\" "
             "vlink=\"#000080\" alink=\"#8B6914\">\n");
    page_add("<iframe src=\"/banner\" name=\"banner\" width=\"100%\" height=\"70\" "
             "scrolling=\"no\" frameborder=\"1\">");
    page_add("<font face=\"Times New Roman, Times, serif\" color=\"#000080\">"
             "<b>Legacy Credit Union HomeBanking</b></font>");
    page_add("</iframe>\n");
    page_add("<font face=\"Times New Roman, Times, serif\">\n");
}

static void page_close(void) {
    page_add("<p><font size=\"2\" color=\"#000080\">Legacy Credit Union"
             " &copy; 2001 &nbsp; Member accounts are posted nightly.</font></p>\n");
    page_add("</font></body></html>\n");
}

static void html_page(int fd) {
    respond(fd, 200, "OK", "text/html", 0, PAGE, page_len);
}

static int header_value(const char* headers, const char* key, char* dest, int cap) {
    int klen = (int)strlen(key);
    const char* p = headers;
    dest[0] = 0;
    while (*p) {
        if (strncasecmp(p, key, klen) == 0 && p[klen] == ':') {
            int n = 0;
            p += klen + 1;
            while (*p == ' ') p++;
            while (*p && *p != '\r' && *p != '\n' && n < cap - 1) dest[n++] = *p++;
            dest[n] = 0;
            return 1;
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    return 0;
}

static void url_decode(const char* in, char* out, int cap) {
    int n = 0;
    while (*in && n < cap - 1) {
        if (*in == '+') {
            out[n++] = ' ';
            in++;
        } else if (*in == '%' && isxdigit((unsigned char)in[1]) &&
                   isxdigit((unsigned char)in[2])) {
            char hex[3];
            hex[0] = in[1];
            hex[1] = in[2];
            hex[2] = 0;
            out[n++] = (char)strtol(hex, 0, 16);
            in += 3;
        } else {
            out[n++] = *in++;
        }
    }
    out[n] = 0;
}

static int form_get(const char* body, const char* key, char* dest, int cap) {
    int klen = (int)strlen(key);
    const char* p = body ? body : "";
    dest[0] = 0;
    while (*p) {
        if ((p == body || p[-1] == '&') && strncmp(p, key, klen) == 0 && p[klen] == '=') {
            char raw[256];
            int n = 0;
            p += klen + 1;
            while (*p && *p != '&' && n < (int)sizeof raw - 1) raw[n++] = *p++;
            raw[n] = 0;
            url_decode(raw, dest, cap);
            return 1;
        }
        while (*p && *p != '&') p++;
        if (*p == '&') p++;
    }
    return 0;
}

static int cookie_member(const char* headers, char* member) {
    char cookie[512];
    const char* p;
    member[0] = 0;
    if (!header_value(headers, "Cookie", cookie, (int)sizeof cookie)) return 0;
    p = strstr(cookie, "LCUMember=");
    if (!p) return 0;
    p += strlen("LCUMember=");
    if (strlen(p) < 6) return 0;
    memcpy(member, p, 6);
    member[6] = 0;
    return 1;
}

static int find_member(const char* num, char* rec_out) {
    char rows[MAX_MBRS][MBR_LEN + 1];
    int n = read_records(PATH_MEMBERS, &rows[0][0], MBR_LEN, MBR_LEN + 1, MAX_MBRS);
    int i;
    if (n < 0) return -1;
    for (i = 0; i < n; i++) {
        if (field_eq(rows[i], MBR_NUM_OFF, MBR_NUM_LEN, num)) {
            memcpy(rec_out, rows[i], MBR_LEN + 1);
            return 1;
        }
    }
    return 0;
}

static int load_accounts(char rows[][ACCT_LEN + 1]) {
    return read_records(PATH_ACCOUNTS, &rows[0][0], ACCT_LEN, ACCT_LEN + 1, MAX_ACCTS);
}

static void serve_banner(int fd) {
    const char* body =
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
        "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
        "<html><head><title>Legacy Credit Union</title></head>\n"
        "<body bgcolor=\"#000080\" text=\"#FFD700\" topmargin=\"8\" leftmargin=\"12\">\n"
        "<font face=\"Times New Roman, Times, serif\" size=\"5\" color=\"#FFD700\">"
        "<b>Legacy Credit Union</b></font>\n"
        "<font face=\"Times New Roman, Times, serif\" size=\"3\" color=\"#FFFFFF\">"
        "&nbsp;&nbsp;HomeBanking</font>\n"
        "</body></html>\n";
    respond(fd, 200, "OK", "text/html", 0, body, (int)strlen(body));
}

static void serve_bankjs(int fd) {
    FILE* f = fopen("web/bank.js", "r");
    char buf[8192];
    int n;
    if (!f) {
        respond(fd, 404, "Not Found", "text/plain", 0, "missing bank.js\n", 15);
        return;
    }
    n = (int)fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    if (n < 0) n = 0;
    buf[n] = 0;
    respond(fd, 200, "OK", "text/javascript", 0, buf, n);
}

static void login_body(void) {
    page_open("Legacy Credit Union HomeBanking Sign On");
    page_add("<h2>Sign On</h2>\n");
    page_add("<table border=\"1\" cellpadding=\"8\" cellspacing=\"0\" bgcolor=\"#FFF8DC\">\n");
    page_add("<tr><td>\n");
    page_add("<b>Demonstration access</b><br>\n");
    page_add("Name: Elena Vasquez<br>\n");
    page_add("Member number: 100042<br>\n");
    page_add("PIN: 2468<br>\n");
    page_add("</td></tr></table>\n");
    page_add("<p>Enter the member number and PIN for your account.</p>\n");
    page_add("<form method=\"post\" action=\"/login\">\n");
    page_add("<table cellpadding=\"4\">\n");
    page_add("<tr><td>Member number</td><td>"
             "<input type=\"text\" name=\"member\" size=\"8\" maxlength=\"6\"></td></tr>\n");
    page_add("<tr><td>PIN</td><td>"
             "<input type=\"password\" name=\"pin\" size=\"8\" maxlength=\"4\"></td></tr>\n");
    page_add("<tr><td></td><td><input type=\"submit\" value=\"Sign On\"></td></tr>\n");
    page_add("</table></form>\n");
    page_close();
}

static void serve_login(int fd) {
    login_body();
    html_page(fd);
}

static int require_member(int fd, const char* headers, char* member, char* mrec) {
    int fr;
    if (!cookie_member(headers, member)) {
        redirect_to(fd, "/", 0);
        return 0;
    }
    fr = find_member(member, mrec);
    if (fr != 1 || mrec[MBR_STAT_OFF] != 'A') {
        redirect_to(fd, "/", "LCUMember=; Path=/");
        return 0;
    }
    return 1;
}

static void nav_bar(void) {
    page_add("<p><a href=\"/accounts\">Accounts</a> &nbsp;|&nbsp; "
             "<a href=\"/transfer\">Transfer</a> &nbsp;|&nbsp; "
             "<a href=\"/signoff\">Sign off</a></p>\n");
}

static void serve_accounts(int fd, const char* headers) {
    char member[8];
    char mrec[MBR_LEN + 1];
    char name[MBR_NAME_LEN + 1];
    char accts[MAX_ACCTS][ACCT_LEN + 1];
    char ctrl[64];
    char stamp[32];
    char money[32];
    char open_d[16];
    FILE* cf;
    int n;
    int i;
    int any = 0;
    if (!require_member(fd, headers, member, mrec)) return;
    field_copy(mrec, MBR_NAME_OFF, MBR_NAME_LEN, name);
    n = load_accounts(accts);
    ctrl[0] = 0;
    cf = fopen(PATH_CONTROL, "r");
    if (cf) {
        if (!fgets(ctrl, (int)sizeof ctrl, cf)) ctrl[0] = 0;
        fclose(cf);
        {
            int L = (int)strlen(ctrl);
            while (L > 0 && (ctrl[L - 1] == '\n' || ctrl[L - 1] == '\r')) ctrl[--L] = 0;
        }
    }
    format_stamp(ctrl, stamp);
    page_open("Accounts");
    page_add("<h2>Accounts</h2>\n");
    page_add("<p>Member ");
    page_esc(member);
    page_add(" &nbsp; ");
    page_esc(name);
    page_add("</p>\n");
    page_add("<p>Last nightly posting: ");
    page_esc(stamp);
    page_add("</p>\n");
    nav_bar();
    page_add("<table border=\"1\" cellpadding=\"5\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
    page_add("<tr bgcolor=\"#000080\">");
    page_add("<td><font color=\"#FFD700\">Account</font></td>");
    page_add("<td><font color=\"#FFD700\">Type</font></td>");
    page_add("<td><font color=\"#FFD700\">Balance</font></td>");
    page_add("<td><font color=\"#FFD700\">Opened</font></td>");
    page_add("<td><font color=\"#FFD700\">Status</font></td>");
    page_add("</tr>\n");
    if (n > 0) {
        for (i = 0; i < n; i++) {
            char anum[ACCT_NUM_LEN + 1];
            char opened[ACCT_OPEN_LEN + 1];
            long bal;
            if (!field_eq(accts[i], ACCT_MBR_OFF, ACCT_MBR_LEN, member)) continue;
            any = 1;
            field_copy(accts[i], ACCT_NUM_OFF, ACCT_NUM_LEN, anum);
            field_copy(accts[i], ACCT_OPEN_OFF, ACCT_OPEN_LEN, opened);
            format_date(opened, open_d);
            bal = field_amount(accts[i], ACCT_BAL_OFF, ACCT_BAL_LEN);
            format_money(money, bal);
            page_add("<tr><td>");
            if (accts[i][ACCT_TYPE_OFF] == 'R') page_add("<a href=\"/holdings?acct=");
            else page_add("<a href=\"/history?acct=");
            page_esc(anum);
            page_add("\">");
            page_esc(anum);
            page_add("</a></td><td>");
            page_esc(acct_type_name(accts[i][ACCT_TYPE_OFF]));
            page_add("</td><td align=\"right\">$");
            page_esc(money);
            page_add("</td><td>");
            page_esc(open_d);
            page_add("</td><td>");
            page_esc(status_name(accts[i][ACCT_STAT_OFF]));
            page_add("</td></tr>\n");
        }
    }
    page_add("</table>\n");
    if (!any) page_add("<p>No accounts are on file for this member.</p>\n");
    page_add("<p>Balances above are from the last nightly posting. "
             "A transfer you submit today stays pending until then.</p>\n");
    page_close();
    html_page(fd);
}

static int member_owns(const char* member, const char* acct, char* rec_out) {
    char accts[MAX_ACCTS][ACCT_LEN + 1];
    int n = load_accounts(accts);
    int i;
    if (n < 0) return -1;
    for (i = 0; i < n; i++) {
        if (field_eq(accts[i], ACCT_NUM_OFF, ACCT_NUM_LEN, acct) &&
            field_eq(accts[i], ACCT_MBR_OFF, ACCT_MBR_LEN, member)) {
            if (rec_out) memcpy(rec_out, accts[i], ACCT_LEN + 1);
            return 1;
        }
    }
    return 0;
}

static void serve_history(int fd, const char* headers, const char* query) {
    char member[8];
    char mrec[MBR_LEN + 1];
    char acct[16];
    char arec[ACCT_LEN + 1];
    char txns[MAX_TXNS][TXN_LEN + 1];
    char money[32];
    char date[16];
    char desc[TXN_DESC_LEN + 1];
    int own;
    int n;
    int i;
    long bal;
    if (!require_member(fd, headers, member, mrec)) return;
    form_get(query, "acct", acct, (int)sizeof acct);
    own = member_owns(member, acct, arec);
    page_open("Transaction history");
    page_add("<h2>Transaction history</h2>\n");
    nav_bar();
    if (own != 1) {
        page_add("<p>That account is not on this membership.</p>\n");
        page_close();
        html_page(fd);
        return;
    }
    bal = field_amount(arec, ACCT_BAL_OFF, ACCT_BAL_LEN);
    format_money(money, bal);
    page_add("<p>");
    page_esc(acct_type_name(arec[ACCT_TYPE_OFF]));
    page_add(" ");
    page_esc(acct);
    page_add(" &nbsp; Posted balance $");
    page_esc(money);
    page_add("</p>\n");
    page_add("<table border=\"1\" cellpadding=\"4\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
    page_add("<tr bgcolor=\"#000080\">");
    page_add("<td><font color=\"#FFD700\">Date</font></td>");
    page_add("<td><font color=\"#FFD700\">Description</font></td>");
    page_add("<td><font color=\"#FFD700\">Debit</font></td>");
    page_add("<td><font color=\"#FFD700\">Credit</font></td>");
    page_add("</tr>\n");
    n = read_records(PATH_TXNS, &txns[0][0], TXN_LEN, TXN_LEN + 1, MAX_TXNS);
    if (n > 0) {
        for (i = 0; i < n; i++) {
            char rawdate[8];
            long amt;
            if (!field_eq(txns[i], TXN_ACCT_OFF, TXN_ACCT_LEN, acct)) continue;
            field_copy(txns[i], TXN_DATE_OFF, TXN_DATE_LEN, rawdate);
            format_date(rawdate, date);
            field_copy(txns[i], TXN_DESC_OFF, TXN_DESC_LEN, desc);
            amt = field_amount(txns[i], TXN_AMT_OFF, TXN_AMT_LEN);
            format_money(money, amt);
            page_add("<tr><td>");
            page_esc(date);
            page_add("</td><td>");
            page_esc(desc);
            page_add("</td><td align=\"right\">");
            if (txns[i][TXN_DC_OFF] == 'D') {
                page_add("$");
                page_esc(money);
            }
            page_add("</td><td align=\"right\">");
            if (txns[i][TXN_DC_OFF] == 'C') {
                page_add("$");
                page_esc(money);
            }
            page_add("</td></tr>\n");
        }
    }
    page_add("</table>\n");
    page_close();
    html_page(fd);
}

static long market_cents(long shares_raw, long nav_cents) {
    double v = (double)shares_raw * (double)nav_cents / (double)POS_SHARES_SCALE;
    if (v >= 0) return (long)(v + 0.5);
    return (long)(v - 0.5);
}

static void format_shares(char* dest, long raw) {
    long whole = raw / POS_SHARES_SCALE;
    long frac = raw % POS_SHARES_SCALE;
    if (frac < 0) frac = -frac;
    sprintf(dest, "%ld.%04ld", whole, frac);
}

static void serve_holdings(int fd, const char* headers, const char* query) {
    char member[8];
    char mrec[MBR_LEN + 1];
    char acct[16];
    char arec[ACCT_LEN + 1];
    char funds[MAX_FUNDS][FUND_LEN + 1];
    char pos[MAX_POS][POS_LEN + 1];
    char money[32];
    char shares[32];
    char price[32];
    int own;
    int nf;
    int np;
    int i;
    int j;
    long total = 0;
    long posted;
    int rows = 0;
    if (!require_member(fd, headers, member, mrec)) return;
    form_get(query, "acct", acct, (int)sizeof acct);
    own = member_owns(member, acct, arec);
    page_open("Retirement holdings");
    page_add("<h2>Retirement holdings</h2>\n");
    nav_bar();
    if (own != 1 || arec[ACCT_TYPE_OFF] != 'R') {
        page_add("<p>Holdings are shown only for a retirement account on this membership.</p>\n");
        page_close();
        html_page(fd);
        return;
    }
    posted = field_amount(arec, ACCT_BAL_OFF, ACCT_BAL_LEN);
    format_money(money, posted);
    page_add("<p>Retirement account ");
    page_esc(acct);
    page_add("</p>\n");
    page_add("<p>This is a credit union retirement plan, not a brokerage account. "
             "To change investments, call your branch. Online trading is not available.</p>\n");
    page_add("<table border=\"1\" cellpadding=\"4\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
    page_add("<tr bgcolor=\"#000080\">");
    page_add("<td><font color=\"#FFD700\">Fund</font></td>");
    page_add("<td><font color=\"#FFD700\">Shares</font></td>");
    page_add("<td><font color=\"#FFD700\">Price</font></td>");
    page_add("<td><font color=\"#FFD700\">Market value</font></td>");
    page_add("</tr>\n");
    nf = read_records(PATH_FUNDS, &funds[0][0], FUND_LEN, FUND_LEN + 1, MAX_FUNDS);
    np = read_records(PATH_POSITIONS, &pos[0][0], POS_LEN, POS_LEN + 1, MAX_POS);
    if (np > 0 && nf > 0) {
        for (i = 0; i < np; i++) {
            char fund_id[8];
            char fname[FUND_NAME_LEN + 1];
            long sh;
            long nav;
            long mv;
            int matched = 0;
            if (!field_eq(pos[i], POS_ACCT_OFF, POS_ACCT_LEN, acct)) continue;
            field_copy(pos[i], POS_FUND_OFF, POS_FUND_LEN, fund_id);
            sh = field_amount(pos[i], POS_SHARES_OFF, POS_SHARES_LEN);
            fname[0] = 0;
            nav = 0;
            for (j = 0; j < nf; j++) {
                if (field_eq(funds[j], FUND_ID_OFF, FUND_ID_LEN, fund_id)) {
                    field_copy(funds[j], FUND_NAME_OFF, FUND_NAME_LEN, fname);
                    nav = field_amount(funds[j], FUND_NAV_OFF, FUND_NAV_LEN);
                    matched = 1;
                    break;
                }
            }
            if (!matched) continue;
            mv = market_cents(sh, nav);
            total += mv;
            rows++;
            format_shares(shares, sh);
            format_money(price, nav);
            format_money(money, mv);
            page_add("<tr><td>");
            page_esc(fname);
            page_add("</td><td align=\"right\">");
            page_esc(shares);
            page_add("</td><td align=\"right\">$");
            page_esc(price);
            page_add("</td><td align=\"right\">$");
            page_esc(money);
            page_add("</td></tr>\n");
        }
    }
    format_money(money, total);
    page_add("<tr><td colspan=\"3\"><b>Total</b></td><td align=\"right\"><b>$");
    page_esc(money);
    page_add("</b></td></tr>\n</table>\n");
    format_money(money, posted);
    page_add("<p>Total market value of holdings: see the total above. "
             "Posted account balance: $");
    page_esc(money);
    page_add(".</p>\n");
    page_add("<p>A contribution or transfer into this account increases the posted "
             "balance when the nightly batch runs. It does not buy fund shares. "
             "Call the branch to allocate a contribution.</p>\n");
    if (rows == 0) page_add("<p>No fund positions are on file.</p>\n");
    page_close();
    html_page(fd);
}

static void serve_transfer_form(int fd, const char* headers) {
    char member[8];
    char mrec[MBR_LEN + 1];
    char accts[MAX_ACCTS][ACCT_LEN + 1];
    int n;
    int i;
    int first = 1;
    if (!require_member(fd, headers, member, mrec)) return;
    n = load_accounts(accts);
    page_open("Transfer funds");
    page_add("<h2>Transfer funds</h2>\n");
    nav_bar();
    page_add("<p>A transfer is held until the nightly batch. "
             "This screen does not move your balance.</p>\n");
    page_add("<script type=\"text/javascript\">\nvar bals = new Array();\n");
    if (n > 0) {
        for (i = 0; i < n; i++) {
            char anum[ACCT_NUM_LEN + 1];
            if (!field_eq(accts[i], ACCT_MBR_OFF, ACCT_MBR_LEN, member)) continue;
            if (accts[i][ACCT_STAT_OFF] != 'A') continue;
            field_copy(accts[i], ACCT_NUM_OFF, ACCT_NUM_LEN, anum);
            page_add("bals[\"");
            page_esc(anum);
            page_add("\"] = ");
            page_addf("%ld", field_amount(accts[i], ACCT_BAL_OFF, ACCT_BAL_LEN));
            page_add(";\n");
        }
    }
    page_add("</script>\n");
    page_add("<script type=\"text/javascript\" src=\"/bank.js\"></script>\n");
    page_add("<form name=\"xfer\" method=\"post\" action=\"/transfer\" "
             "onsubmit=\"return checkTransfer();\">\n");
    page_add("<table cellpadding=\"4\">\n");
    page_add("<tr><td>From</td><td><select name=\"fromacct\" onchange=\"syncBal();\">\n");
    if (n > 0) {
        for (i = 0; i < n; i++) {
            char anum[ACCT_NUM_LEN + 1];
            char money[32];
            long bal;
            if (!field_eq(accts[i], ACCT_MBR_OFF, ACCT_MBR_LEN, member)) continue;
            if (accts[i][ACCT_STAT_OFF] != 'A') continue;
            field_copy(accts[i], ACCT_NUM_OFF, ACCT_NUM_LEN, anum);
            bal = field_amount(accts[i], ACCT_BAL_OFF, ACCT_BAL_LEN);
            format_money(money, bal);
            page_add("<option value=\"");
            page_esc(anum);
            page_add("\"");
            if (first) {
                page_add(" selected");
                first = 0;
            }
            page_add(">");
            page_esc(acct_type_name(accts[i][ACCT_TYPE_OFF]));
            page_add(" ");
            page_esc(anum);
            page_add(" $");
            page_esc(money);
            page_add("</option>\n");
        }
    }
    page_add("</select></td></tr>\n");
    page_add("<tr><td>To</td><td><select name=\"toacct\">\n");
    if (n > 0) {
        for (i = 0; i < n; i++) {
            char anum[ACCT_NUM_LEN + 1];
            if (!field_eq(accts[i], ACCT_MBR_OFF, ACCT_MBR_LEN, member)) continue;
            if (accts[i][ACCT_STAT_OFF] != 'A') continue;
            field_copy(accts[i], ACCT_NUM_OFF, ACCT_NUM_LEN, anum);
            page_add("<option value=\"");
            page_esc(anum);
            page_add("\">");
            page_esc(acct_type_name(accts[i][ACCT_TYPE_OFF]));
            page_add(" ");
            page_esc(anum);
            page_add("</option>\n");
        }
    }
    page_add("</select></td></tr>\n");
    page_add("<tr><td>Amount</td><td>"
             "<input type=\"text\" name=\"amount\" size=\"12\" maxlength=\"12\"></td></tr>\n");
    page_add("<tr><td>Description</td><td>"
             "<input type=\"text\" name=\"desc\" size=\"24\" maxlength=\"24\"></td></tr>\n");
    page_add("<tr><td></td><td><input type=\"hidden\" name=\"shownbal\" value=\"0\">"
             "<input type=\"submit\" value=\"Schedule transfer\"></td></tr>\n");
    page_add("</table></form>\n");
    page_add("<script type=\"text/javascript\">syncBal();</script>\n");
    page_close();
    html_page(fd);
}

static void serve_transfer_post(int fd, const char* headers, const char* body) {
    char member[8];
    char mrec[MBR_LEN + 1];
    char from_acct[16];
    char to_acct[16];
    char amount[32];
    char desc[64];
    char from_rec[ACCT_LEN + 1];
    char to_rec[ACCT_LEN + 1];
    long cents = 0;
    if (!require_member(fd, headers, member, mrec)) return;
    form_get(body, "fromacct", from_acct, (int)sizeof from_acct);
    form_get(body, "toacct", to_acct, (int)sizeof to_acct);
    form_get(body, "amount", amount, (int)sizeof amount);
    form_get(body, "desc", desc, (int)sizeof desc);
    page_open("Transfer scheduled");
    page_add("<h2>Transfer</h2>\n");
    nav_bar();
    if (member_owns(member, from_acct, from_rec) != 1 ||
        member_owns(member, to_acct, to_rec) != 1) {
        page_add("<p>Choose two accounts on this membership.</p>\n");
        page_close();
        html_page(fd);
        return;
    }
    if (strcmp(from_acct, to_acct) == 0 || dollars_to_cents(amount, &cents) != 0 || cents <= 0) {
        page_add("<p>Enter two different accounts and an amount greater than zero.</p>\n");
        page_close();
        html_page(fd);
        return;
    }
    /* Balance is not checked here. web/bank.js does that before submit. */
    if (desc[0] == 0) strcpy(desc, "WEB TRANSFER");
    if (append_pending('W', "0000", "XFR", from_acct, to_acct, cents, desc) != 0) {
        page_add("<p>The transfer could not be recorded. Please call the branch.</p>\n");
        page_close();
        html_page(fd);
        return;
    }
    page_add("<p>Your transfer has been scheduled. It will post with the nightly batch. "
             "Your balance has not changed.</p>\n");
    page_add("<table border=\"1\" cellpadding=\"4\" bgcolor=\"#FFFFFF\">\n<tr><td>From</td><td>");
    page_esc(from_acct);
    page_add("</td></tr><tr><td>To</td><td>");
    page_esc(to_acct);
    page_add("</td></tr><tr><td>Amount</td><td>$");
    page_esc(amount);
    page_add("</td></tr><tr><td>Description</td><td>");
    page_esc(desc);
    page_add("</td></tr></table>\n");
    page_close();
    html_page(fd);
}

static void handle_login_post(int fd, const char* body) {
    char member[16];
    char pin[16];
    char rec[MBR_LEN + 1];
    char cookie[64];
    int fr;
    form_get(body, "member", member, (int)sizeof member);
    form_get(body, "pin", pin, (int)sizeof pin);
    fr = find_member(member, rec);
    if (fr == 1 && rec[MBR_STAT_OFF] == 'A' &&
        field_eq(rec, MBR_PIN_OFF, MBR_PIN_LEN, pin)) {
        sprintf(cookie, "LCUMember=%s; Path=/", member);
        redirect_to(fd, "/accounts", cookie);
        return;
    }
    page_open("Sign on failed");
    page_add("<h2>Sign on failed</h2>\n");
    if (fr == 1 && rec[MBR_STAT_OFF] != 'A') {
        page_add("<p>This member number is not active. Please call the branch.</p>\n");
    } else {
        page_add("<p>The member number or PIN is not valid.</p>\n");
    }
    page_add("<p><a href=\"/\">Return to sign on</a></p>\n");
    page_close();
    html_page(fd);
}

static int read_request(int fd, char* buf, int cap, int* header_len, char** body) {
    int n = 0;
    int clen = 0;
    char cl[32];
    char* p;
    buf[0] = 0;
    while (n < cap - 1) {
        int r = (int)recv(fd, buf + n, cap - 1 - n, 0);
        if (r < 0) return -1;
        if (r == 0) break;
        n += r;
        buf[n] = 0;
        p = strstr(buf, "\r\n\r\n");
        if (!p) continue;
        *header_len = (int)(p - buf);
        *body = p + 4;
        clen = 0;
        if (header_value(buf, "Content-Length", cl, (int)sizeof cl)) clen = atoi(cl);
        if (clen < 0) clen = 0;
        if (clen > 8192) clen = 8192;
        while (n < *header_len + 4 + clen && n < cap - 1) {
            r = (int)recv(fd, buf + n, cap - 1 - n, 0);
            if (r <= 0) break;
            n += r;
            buf[n] = 0;
        }
        return n;
    }
    return -1;
}

static void handle_client(int fd) {
    char buf[16384];
    char method[16];
    char target[256];
    char path[256];
    char* query = 0;
    char* body = 0;
    char* sp;
    int header_len = 0;
    int i;
    const char* headers;
    if (read_request(fd, buf, (int)sizeof buf, &header_len, &body) < 0) return;
    headers = buf;
    method[0] = 0;
    target[0] = 0;
    sp = strchr(buf, ' ');
    if (!sp) return;
    if ((int)(sp - buf) >= (int)sizeof method) return;
    memcpy(method, buf, sp - buf);
    method[sp - buf] = 0;
    sp++;
    i = 0;
    while (*sp && *sp != ' ' && i < (int)sizeof target - 1) target[i++] = *sp++;
    target[i] = 0;
    strncpy(path, target, sizeof path - 1);
    path[sizeof path - 1] = 0;
    query = strchr(path, '?');
    if (query) {
        *query = 0;
        query++;
    } else {
        query = (char*)"";
    }
    fprintf(stderr, "%s %s\n", method, target);
    if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0) serve_login(fd);
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/login") == 0) serve_login(fd);
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/banner") == 0) serve_banner(fd);
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/bank.js") == 0) serve_bankjs(fd);
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/login") == 0) handle_login_post(fd, body);
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/accounts") == 0) serve_accounts(fd, headers);
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/history") == 0) serve_history(fd, headers, query);
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/holdings") == 0) serve_holdings(fd, headers, query);
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/transfer") == 0) serve_transfer_form(fd, headers);
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/transfer") == 0) serve_transfer_post(fd, headers, body);
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/signoff") == 0) {
        redirect_to(fd, "/", "LCUMember=; Path=/");
    } else {
        const char* msg = "<html><body><p>The page was not found.</p>"
                          "<p><a href=\"/\">Sign on</a></p></body></html>";
        respond(fd, 404, "Not Found", "text/html", 0, msg, (int)strlen(msg));
    }
}

int main(void) {
    int srv;
    int opt = 1;
    struct sockaddr_in addr;
    signal(SIGPIPE, SIG_IGN);
    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        perror("socket");
        return 1;
    }
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    if (inet_aton("127.0.0.1", &addr.sin_addr) == 0) {
        fprintf(stderr, "bind address\n");
        return 1;
    }
    if (bind(srv, (struct sockaddr*)&addr, sizeof addr) != 0) {
        perror("bind");
        return 1;
    }
    if (listen(srv, 8) != 0) {
        perror("listen");
        return 1;
    }
    fprintf(stderr, "listening on 127.0.0.1:8080\n");
    for (;;) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof cli;
        int fd = accept(srv, (struct sockaddr*)&cli, &clilen);
        struct timeval tv;
        if (fd < 0) continue;
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        handle_client(fd);
        close(fd);
    }
    return 0;
}
