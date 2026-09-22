#include "layout.h"

#include <ncurses.h>
#include <signal.h>
#include <unistd.h>

enum {
    MAX_ACCTS = 80,
    MAX_MBRS = 32,
    MAX_TELLERS = 16,
    POST_OK = 0,
    POST_COURTESY = 1,
    POST_NOLOGIN = 2,
    POST_NOACCT = 3,
    POST_INACTIVE = 4,
    POST_NSF = 5,
    POST_BADAMT = 6,
    POST_SAME = 7,
    POST_IO = 8
};

static int g_errors = 0;
static int g_logged = 0;
static char g_teller[TLR_ID_LEN + 1];
static char g_tname[TLR_NAME_LEN + 1];

static void say(const char* s) {
    printf("%s\n", s);
    fflush(stdout);
}

static void rtrim_inplace(char* s) {
    int n = (int)strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ')) {
        s[--n] = 0;
    }
}

static int load_table(const char* path, char* storage, int rec_len, int maxn) {
    int n = read_records(path, storage, rec_len, rec_len + 1, maxn);
    return n;
}

static int lookup_account(const char* acct, char* rec_out) {
    char rows[MAX_ACCTS][ACCT_LEN + 1];
    int n = load_table(PATH_ACCOUNTS, &rows[0][0], ACCT_LEN, MAX_ACCTS);
    int i;
    if (n < 0) return -1;
    for (i = 0; i < n; i++) {
        if (field_eq(rows[i], ACCT_NUM_OFF, ACCT_NUM_LEN, acct)) {
            memcpy(rec_out, rows[i], ACCT_LEN + 1);
            return 1;
        }
    }
    return 0;
}

/* Courtesy pay of $50 is enforced only on draft withdrawals, and only here. */
static int post_activity(const char* code, const char* from_acct, const char* to_acct,
                         long cents, const char* desc) {
    char from_rec[ACCT_LEN + 1];
    char to_rec[ACCT_LEN + 1];
    int fr;
    int to = 1;
    char type;
    char status;
    long bal;
    int courtesy = 0;
    if (!g_logged) return POST_NOLOGIN;
    if (cents <= 0) return POST_BADAMT;
    if (strcmp(code, "XFR") == 0 && strcmp(from_acct, to_acct) == 0) return POST_SAME;
    fr = lookup_account(from_acct, from_rec);
    if (fr < 0) return POST_IO;
    if (fr == 0) return POST_NOACCT;
    type = from_rec[ACCT_TYPE_OFF];
    status = from_rec[ACCT_STAT_OFF];
    bal = field_amount(from_rec, ACCT_BAL_OFF, ACCT_BAL_LEN);
    if (status != 'A') return POST_INACTIVE;
    if (strcmp(code, "XFR") == 0) {
        to = lookup_account(to_acct, to_rec);
        if (to < 0) return POST_IO;
        if (to == 0) return POST_NOACCT;
        if (to_rec[ACCT_STAT_OFF] != 'A') return POST_INACTIVE;
    }
    if (strcmp(code, "WDL") == 0 || strcmp(code, "XFR") == 0) {
        if (cents > bal) {
            if (strcmp(code, "WDL") == 0 && type == 'D' && (cents - bal) <= 5000) {
                courtesy = 1;
            } else {
                return POST_NSF;
            }
        }
    }
    if (append_pending('T', g_teller, code, from_acct,
                       (strcmp(code, "XFR") == 0) ? to_acct : "",
                       cents, desc) != 0) {
        return POST_IO;
    }
    return courtesy ? POST_COURTESY : POST_OK;
}

static const char* post_msg(int rc) {
    if (rc == POST_OK) return "OK";
    if (rc == POST_COURTESY) return "OK";
    if (rc == POST_NOLOGIN) return "ERR LOGIN";
    if (rc == POST_NOACCT) return "ERR NOACCT";
    if (rc == POST_INACTIVE) return "ERR INACTIVE";
    if (rc == POST_NSF) return "ERR NSF";
    if (rc == POST_BADAMT) return "ERR AMOUNT";
    if (rc == POST_SAME) return "ERR SAME";
    return "ERR IO";
}

static int do_login(const char* id, const char* pw) {
    char rows[MAX_TELLERS][TLR_LEN + 1];
    char padded[TLR_PASS_LEN + 1];
    int n;
    int i;
    int plen;
    memset(padded, ' ', TLR_PASS_LEN);
    padded[TLR_PASS_LEN] = 0;
    if (!id || !pw) return 0;
    plen = (int)strlen(pw);
    if (strlen(id) != TLR_ID_LEN || plen > TLR_PASS_LEN) return 0;
    memcpy(padded, pw, plen);
    n = load_table(PATH_TELLERS, &rows[0][0], TLR_LEN, MAX_TELLERS);
    if (n < 0) return 0;
    for (i = 0; i < n; i++) {
        if (field_eq(rows[i], TLR_ID_OFF, TLR_ID_LEN, id) &&
            memcmp(rows[i] + TLR_PASS_OFF, padded, TLR_PASS_LEN) == 0) {
            memcpy(g_teller, id, TLR_ID_LEN);
            g_teller[TLR_ID_LEN] = 0;
            field_copy(rows[i], TLR_NAME_OFF, TLR_NAME_LEN, g_tname);
            g_logged = 1;
            return 1;
        }
    }
    return 0;
}

static void print_member(const char* member) {
    char mbrs[MAX_MBRS][MBR_LEN + 1];
    char accts[MAX_ACCTS][ACCT_LEN + 1];
    char name[MBR_NAME_LEN + 1];
    char line[128];
    char money_digits[16];
    int nm;
    int na;
    int i;
    int found = 0;
    nm = load_table(PATH_MEMBERS, &mbrs[0][0], MBR_LEN, MAX_MBRS);
    na = load_table(PATH_ACCOUNTS, &accts[0][0], ACCT_LEN, MAX_ACCTS);
    if (nm < 0 || na < 0) {
        say("ERR IO");
        g_errors++;
        return;
    }
    for (i = 0; i < nm; i++) {
        if (field_eq(mbrs[i], MBR_NUM_OFF, MBR_NUM_LEN, member)) {
            field_copy(mbrs[i], MBR_NAME_OFF, MBR_NAME_LEN, name);
            sprintf(line, "OK FIND %s", member);
            say(line);
            sprintf(line, "NAME %s", name);
            say(line);
            sprintf(line, "STATUS %c", mbrs[i][MBR_STAT_OFF]);
            say(line);
            found = 1;
            break;
        }
    }
    if (!found) {
        say("ERR NOACCT");
        g_errors++;
        return;
    }
    for (i = 0; i < na; i++) {
        if (!field_eq(accts[i], ACCT_MBR_OFF, ACCT_MBR_LEN, member)) continue;
        sprintf(money_digits, "%.*s", ACCT_BAL_LEN, accts[i] + ACCT_BAL_OFF);
        sprintf(line, "ACCT %.*s %c %s %c",
                ACCT_NUM_LEN, accts[i] + ACCT_NUM_OFF,
                accts[i][ACCT_TYPE_OFF],
                money_digits,
                accts[i][ACCT_STAT_OFF]);
        say(line);
    }
}

static char* next_tok(char** p) {
    char* s = *p;
    char* start;
    while (*s == ' ') s++;
    if (*s == 0) {
        *p = s;
        return 0;
    }
    start = s;
    while (*s && *s != ' ') s++;
    if (*s) {
        *s = 0;
        s++;
    }
    *p = s;
    return start;
}

static void rest_desc(char** p, char* desc, int maxlen) {
    int n;
    while (**p == ' ') (*p)++;
    n = (int)strlen(*p);
    if (n > maxlen) n = maxlen;
    memcpy(desc, *p, n);
    desc[n] = 0;
    rtrim_inplace(desc);
}

static void report_post(int rc, const char* code, const char* acct, long cents) {
    char line[128];
    if (rc == POST_OK) {
        sprintf(line, "OK %s %s %011ld", code, acct, cents);
        say(line);
    } else if (rc == POST_COURTESY) {
        sprintf(line, "OK %s %s %011ld COURTESY", code, acct, cents);
        say(line);
    } else {
        say(post_msg(rc));
        g_errors++;
    }
}

static void handle_command(char* line) {
    char* p = line;
    char* verb;
    char* a;
    char* b;
    char* c;
    char desc[TXN_DESC_LEN + 1];
    long cents = 0;
    int rc;
    verb = next_tok(&p);
    if (!verb) return;
    if (strcmp(verb, "QUIT") == 0) return;
    if (strcmp(verb, "LOGIN") == 0) {
        a = next_tok(&p);
        b = next_tok(&p);
        if (a && b && do_login(a, b)) {
            char msg[64];
            sprintf(msg, "OK LOGIN %s", g_teller);
            say(msg);
        } else {
            say("ERR LOGIN");
            g_errors++;
        }
        return;
    }
    if (strcmp(verb, "FIND") == 0) {
        a = next_tok(&p);
        if (!g_logged) {
            say("ERR LOGIN");
            g_errors++;
            return;
        }
        if (!a) {
            say("ERR NOACCT");
            g_errors++;
            return;
        }
        print_member(a);
        return;
    }
    if (strcmp(verb, "DEP") == 0 || strcmp(verb, "WDL") == 0) {
        a = next_tok(&p);
        b = next_tok(&p);
        rest_desc(&p, desc, TXN_DESC_LEN);
        if (!a || !b || dollars_to_cents(b, &cents) != 0) {
            say("ERR AMOUNT");
            g_errors++;
            return;
        }
        rc = post_activity(verb, a, "", cents, desc);
        report_post(rc, verb, a, cents);
        return;
    }
    if (strcmp(verb, "XFR") == 0) {
        a = next_tok(&p);
        b = next_tok(&p);
        c = next_tok(&p);
        rest_desc(&p, desc, TXN_DESC_LEN);
        if (!a || !b || !c || dollars_to_cents(c, &cents) != 0) {
            say("ERR AMOUNT");
            g_errors++;
            return;
        }
        rc = post_activity("XFR", a, b, cents, desc);
        report_post(rc, "XFR", a, cents);
        return;
    }
    say("ERR UNKNOWN");
    g_errors++;
}

static int command_mode(void) {
    char line[256];
    while (fgets(line, (int)sizeof line, stdin)) {
        rtrim_inplace(line);
        if (strcmp(line, "QUIT") == 0) break;
        handle_command(line);
    }
    return g_errors ? 1 : 0;
}

static void ui_die(int sig) {
    (void)sig;
    endwin();
    _exit(1);
}

static int read_field(int y, int x, char* buf, int maxlen, int secret) {
    int i = 0;
    buf[0] = 0;
    move(y, x);
    while (1) {
        int ch = getch();
        if (ch == KEY_F(10)) return -2;
        if (ch == 27) return -1;
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) break;
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (i > 0) {
                i--;
                buf[i] = 0;
                move(y, x + i);
                addch(' ');
                move(y, x + i);
            }
            continue;
        }
        if (ch >= 32 && ch < 127 && i < maxlen) {
            buf[i++] = (char)ch;
            buf[i] = 0;
            addch(secret ? '*' : (chtype)ch);
        }
    }
    return 0;
}

static void paint_banner(const char* title) {
    clear();
    attron(A_BOLD);
    mvprintw(0, 2, "LEGACY CREDIT UNION");
    attroff(A_BOLD);
    mvprintw(1, 2, "TELLER WORKSTATION");
    mvprintw(2, 2, "%s", title);
    if (g_logged) mvprintw(0, 40, "TELLER %s", g_teller);
    mvhline(3, 0, ACS_HLINE, 78);
}

static void pause_msg(const char* msg) {
    mvprintw(20, 2, "%s", msg);
    mvprintw(21, 2, "ENTER TO CONTINUE");
    refresh();
    while (1) {
        int ch = getch();
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) break;
        if (ch == KEY_F(10)) break;
    }
}

static int prompt_money(const char* title, const char* code, int is_xfr) {
    char acct[16];
    char acct2[16];
    char amt[16];
    char desc[TXN_DESC_LEN + 1];
    long cents = 0;
    int rc;
    int rd;
    paint_banner(title);
    mvprintw(5, 2, "ACCOUNT");
    mvprintw(6, 2, "Pending until the nightly batch. The balance will not move.");
    refresh();
    rd = read_field(5, 16, acct, 8, 0);
    if (rd != 0) return rd;
    if (is_xfr) {
        mvprintw(8, 2, "TO ACCOUNT");
        refresh();
        rd = read_field(8, 16, acct2, 8, 0);
        if (rd != 0) return rd;
    } else {
        acct2[0] = 0;
    }
    mvprintw(10, 2, "AMOUNT");
    refresh();
    rd = read_field(10, 16, amt, 12, 0);
    if (rd != 0) return rd;
    mvprintw(12, 2, "DESCRIPTION");
    refresh();
    rd = read_field(12, 16, desc, TXN_DESC_LEN, 0);
    if (rd != 0) return rd;
    if (dollars_to_cents(amt, &cents) != 0) {
        pause_msg("AMOUNT NOT UNDERSTOOD");
        return 0;
    }
    rc = post_activity(code, acct, acct2, cents, desc);
    if (rc == POST_OK) pause_msg("ACCEPTED - PENDING NIGHTLY POSTING. BALANCE UNCHANGED.");
    else if (rc == POST_COURTESY) pause_msg("COURTESY PAY - PENDING. BATCH MAY REJECT.");
    else if (rc == POST_NSF) pause_msg("DECLINED — EXCEEDS 50.00 COURTESY PAY.");
    else pause_msg(post_msg(rc));
    return 0;
}

/* 1 = find another member, 0 = return to the main menu. */
static int show_member_screen(const char* member) {
    char mbrs[MAX_MBRS][MBR_LEN + 1];
    char accts[MAX_ACCTS][ACCT_LEN + 1];
    char name[MBR_NAME_LEN + 1];
    char money[32];
    int nm;
    int na;
    int i;
    int row;
    int found = 0;
    char choice[4];
    nm = load_table(PATH_MEMBERS, &mbrs[0][0], MBR_LEN, MAX_MBRS);
    na = load_table(PATH_ACCOUNTS, &accts[0][0], ACCT_LEN, MAX_ACCTS);
    if (nm < 0 || na < 0) {
        pause_msg("FILE ERROR");
        return 0;
    }
    for (i = 0; i < nm; i++) {
        if (field_eq(mbrs[i], MBR_NUM_OFF, MBR_NUM_LEN, member)) {
            field_copy(mbrs[i], MBR_NAME_OFF, MBR_NAME_LEN, name);
            found = 1;
            paint_banner("MEMBER");
            mvprintw(5, 2, "MEMBER %s  %s  STATUS %c", member, name, mbrs[i][MBR_STAT_OFF]);
            break;
        }
    }
    if (!found) {
        pause_msg("MEMBER NOT ON FILE");
        return 0;
    }
    row = 7;
    for (i = 0; i < na; i++) {
        long bal;
        if (!field_eq(accts[i], ACCT_MBR_OFF, ACCT_MBR_LEN, member)) continue;
        bal = field_amount(accts[i], ACCT_BAL_OFF, ACCT_BAL_LEN);
        format_money(money, bal);
        mvprintw(row++, 2, "%.*s  %-14s %12s  %c",
                 ACCT_NUM_LEN, accts[i] + ACCT_NUM_OFF,
                 acct_type_name(accts[i][ACCT_TYPE_OFF]),
                 money,
                 accts[i][ACCT_STAT_OFF]);
    }
    mvprintw(16, 2, "1 DEPOSIT   2 WITHDRAWAL   3 TRANSFER");
    mvprintw(17, 2, "4 FIND ANOTHER   5 MAIN MENU");
    mvprintw(19, 2, "CHOICE");
    refresh();
    if (read_field(19, 12, choice, 1, 0) != 0) return 0;
    if (choice[0] == '1') prompt_money("DEPOSIT", "DEP", 0);
    else if (choice[0] == '2') prompt_money("WITHDRAWAL", "WDL", 0);
    else if (choice[0] == '3') prompt_money("TRANSFER", "XFR", 1);
    else if (choice[0] == '4') return 1;
    return 0;
}

static int curses_mode(void) {
    char id[8];
    char pw[12];
    char member[8];
    char choice[4];
    int rd;
    signal(SIGINT, ui_die);
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        bkgd(COLOR_PAIR(1));
        attron(COLOR_PAIR(1));
    }
    while (!g_logged) {
        paint_banner("SIGN ON");
        mvprintw(6, 2, "TELLER ID");
        mvprintw(8, 2, "PASSWORD");
        mvprintw(12, 2, "F10 EXIT");
        refresh();
        rd = read_field(6, 16, id, 4, 0);
        if (rd == -2) {
            endwin();
            return 0;
        }
        rd = read_field(8, 16, pw, 8, 1);
        if (rd == -2) {
            endwin();
            return 0;
        }
        if (!do_login(id, pw)) pause_msg("SIGN ON REJECTED");
    }
    while (1) {
        paint_banner("MAIN MENU");
        mvprintw(6, 2, "TELLER %s  %s", g_teller, g_tname);
        mvprintw(8, 2, "1 FIND MEMBER");
        mvprintw(9, 2, "2 SIGN OFF");
        mvprintw(12, 2, "CHOICE");
        refresh();
        rd = read_field(12, 12, choice, 1, 0);
        if (rd == -2 || choice[0] == '2') break;
        if (choice[0] != '1') continue;
        while (1) {
            paint_banner("FIND MEMBER");
            mvprintw(6, 2, "MEMBER NUMBER");
            refresh();
            rd = read_field(6, 18, member, 6, 0);
            if (rd != 0) break;
            if (!show_member_screen(member)) break;
        }
    }
    endwin();
    return 0;
}

int main(void) {
    g_teller[0] = 0;
    g_tname[0] = 0;
    if (!isatty(0)) return command_mode();
    return curses_mode();
}
