/* One-shot generator for the committed data/ history.
   Not part of `make`. Run from the repository root:
       g++ -std=c++98 -Wall -O2 -o /tmp/seedhist src/cpp/seedhist.cpp && /tmp/seedhist
   Rewrites Elena's posted history, balances, retirement files, the widened
   member file, and payees. Other members' short ledgers stay as they were.
   Dividend-reinvest credits on the retirement account are the plug that makes
   the cash balance equal shares times NAV. Price appreciation has nowhere
   else to land in a file that only stores a single balance. */
#include "layout.h"

#include <stdlib.h>

enum { MAX_GEN = 12000, MAX_KEEP = 64, N_MONTHS = 120, N_FUNDS = 4 };

struct GenTxn {
    int date;
    int ord;
    int keep;
    char raw[TXN_LEN + 1];
    char acct[9];
    char dc;
    long cents;
    char desc[25];
};

static GenTxn g_tx[MAX_GEN];
static int g_ntx;
static int g_ord;
static unsigned long g_rng = 20260915UL;

static const char* FUND_ID[N_FUNDS] = {"VFIAX", "VBTLX", "VTIAX", "VMFXX"};
static const char* FUND_NAME[N_FUNDS] = {
    "Vanguard 500 Index Fund Admiral",
    "Vanguard Total Bond Market Index Admiral",
    "Vanguard Total International Stock Index Admiral",
    "Vanguard Federal Money Market"
};

static int nav_at[N_FUNDS][N_MONTHS];
static int fund_whole[N_FUNDS];
static long fund_mv[N_FUNDS];

static int rnd(int n) {
    g_rng = g_rng * 1103515245UL + 12345UL;
    if (n <= 0) return 0;
    return (int)((g_rng >> 16) % (unsigned)n);
}

static int dim(int y, int m) {
    static const int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int d = mdays[m - 1];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) d = 29;
    return d;
}

static int yymmdd_of(int y, int m, int d) {
    return (y % 100) * 10000 + m * 100 + d;
}

static void month_of(int i, int* y, int* m) {
    int idx = (2016 * 12 + 9) + i;
    *y = idx / 12;
    *m = (idx % 12) + 1;
}

static int month_last_day(int y, int m) {
    if (y == 2026 && m == 9) return 15;
    return dim(y, m);
}

static void add_days(int* y, int* m, int* d, int n) {
    while (n > 0) {
        int md = dim(*y, *m);
        if (*d + n <= md) {
            *d += n;
            return;
        }
        n -= (md - *d + 1);
        *d = 1;
        *m += 1;
        if (*m > 12) {
            *m = 1;
            *y += 1;
        }
    }
}

static void add_built(int date, const char* acct, char dc, long cents, const char* desc) {
    GenTxn* t;
    if (cents <= 0) return;
    if (g_ntx >= MAX_GEN) {
        fprintf(stderr, "txn table full\n");
        exit(1);
    }
    t = &g_tx[g_ntx++];
    memset(t, 0, sizeof *t);
    t->date = date;
    t->ord = g_ord++;
    t->keep = 0;
    strncpy(t->acct, acct, 8);
    t->dc = dc;
    t->cents = cents;
    strncpy(t->desc, desc, 24);
}

static int cmp_txn(const void* a, const void* b) {
    const GenTxn* x = (const GenTxn*)a;
    const GenTxn* y = (const GenTxn*)b;
    if (x->date != y->date) return x->date - y->date;
    return x->ord - y->ord;
}

static long market_cents(long shares_raw, long nav_cents) {
    long long num = (long long)shares_raw * (long long)nav_cents;
    if (num >= 0) return (long)((num + (POS_SHARES_SCALE / 2)) / POS_SHARES_SCALE);
    return (long)((num - (POS_SHARES_SCALE / 2)) / POS_SHARES_SCALE);
}

static void put_num(char* rec, int off, int len, long v) {
    char buf[32];
    sprintf(buf, "%0*ld", len, v);
    if ((int)strlen(buf) != len) {
        fprintf(stderr, "number %ld does not fit in %d\n", v, len);
        exit(1);
    }
    memcpy(rec + off, buf, len);
}

static int nav_grown(int i, long start, int bps, int salt) {
    long v = start;
    int k;
    for (k = 0; k < i; k++) {
        v = v * (10000 + bps) / 10000;
        v += ((k * 17 + salt) % 41) - 20;
        if (k == 40) v = v * 78 / 100;
        if (k == 41) v = v * 114 / 100;
        if (k >= 66 && k <= 76) v = v * 988 / 1000;
        if (v < start / 2) v = start / 2;
    }
    return (int)v;
}

static int nav_bond(int i) {
    int base = 1078 - (i * 95) / 119;
    int chop = ((i * 19 + 7) % 23) - 11;
    int dip = 0;
    int v;
    if (i >= 64 && i <= 82) {
        int d = (i <= 73) ? (i - 64) : (82 - i);
        dip = d * 4;
    }
    v = base + chop - dip;
    if (v < 900) v = 900;
    return v;
}

static void build_navs(void) {
    int i;
    int f;
    for (i = 0; i < N_MONTHS; i++) {
        nav_at[0][i] = nav_grown(i, 19840, 95, 3);
        nav_at[1][i] = nav_bond(i);
        nav_at[2][i] = nav_grown(i, 2560, 50, 11);
        nav_at[3][i] = 100;
    }
    for (f = 0; f < N_FUNDS; f++) {
        printf("nav %s %d -> %d\n", FUND_ID[f], nav_at[f][0], nav_at[f][N_MONTHS - 1]);
    }
}

static void choose_shares(void) {
    /* Targets in cents. Whole shares so the latest value is exact. */
    static const long target[N_FUNDS] = {4500000, 1200000, 1600000, 600000};
    int f;
    for (f = 0; f < N_FUNDS; f++) {
        int nav = nav_at[f][N_MONTHS - 1];
        int whole = (int)(target[f] / nav);
        if (whole < 1) whole = 1;
        fund_whole[f] = whole;
        fund_mv[f] = (long)whole * (long)nav;
        printf("fund %s shares %d mv %ld\n", FUND_ID[f], whole, fund_mv[f]);
    }
}

static long retirement_mv(void) {
    long s = 0;
    int f;
    for (f = 0; f < N_FUNDS; f++) s += fund_mv[f];
    return s;
}

static void load_kept_txns(void) {
    FILE* f = fopen(PATH_TXNS, "r");
    char line[256];
    if (!f) {
        fprintf(stderr, "no %s\n", PATH_TXNS);
        exit(1);
    }
    while (fgets(line, (int)sizeof line, f)) {
        int L = (int)strlen(line);
        GenTxn* t;
        while (L > 0 && (line[L - 1] == '\n' || line[L - 1] == '\r')) line[--L] = 0;
        if (L == 0) continue;
        if (L != TXN_LEN) {
            fprintf(stderr, "existing txn length %d\n", L);
            exit(1);
        }
        if (memcmp(line + TXN_ACCT_OFF, "100042", 6) == 0) continue;
        if (g_ntx >= MAX_GEN) exit(1);
        t = &g_tx[g_ntx++];
        memset(t, 0, sizeof *t);
        t->keep = 1;
        memcpy(t->raw, line, TXN_LEN);
        t->raw[TXN_LEN] = 0;
        t->date = (int)field_amount(line, TXN_DATE_OFF, TXN_DATE_LEN);
        t->ord = g_ord++;
    }
    fclose(f);
}

static int is_payroll(int date, const int* pays, int np) {
    int i;
    for (i = 0; i < np; i++) {
        if (pays[i] == date) return 1;
    }
    return 0;
}

static void debit_draft(long* bal, int date, long cents, const char* desc) {
    if (cents <= 0) return;
    if (*bal - cents < 2500) return;
    add_built(date, "10004202", 'D', cents, desc);
    *bal -= cents;
}

static void card_draft(long* bal, int date, const char* merchant, int lo, int hi, int* seq) {
    char desc[32];
    int span = hi - lo + 1;
    long cents;
    if (span < 1) span = 1;
    cents = lo + rnd(span);
    sprintf(desc, "%s %d", merchant, *seq);
    desc[24] = 0;
    *seq += 1;
    debit_draft(bal, date, cents, desc);
}

static void build_elena_activity(void) {
    int pays[400];
    int np = 0;
    int y = 2016;
    int m = 10;
    int d = 7;
    int end = yymmdd_of(2026, 9, 15);
    int i;
    int card = 1000;
    int checkno = 2100;
    long draft = 0;
    long share = 0;
    long loan;
    long final_mv;
    long contrib_total;
    long div_total;
    long div_each;
    long div_used = 0;
    const char* groc[] = {"KROGER", "FRYS", "SAFEWAY", "WALMART", "TARGET", "COSTCO"};
    const char* fuel[] = {"SHELL", "CHEVRON", "CIRCLEK", "ARCO", "QTMART"};
    const char* eat[] = {"STARBUCKS", "MCDONALDS", "CHIPOTLE", "PANERA"};
    const char* shop[] = {"AMAZON", "HOMEDEPOT", "LOWES", "ROSS", "CVS", "WALGREENS", "NETFLIX"};

    add_built(yymmdd_of(2016, 9, 16), "10004202", 'C', 150000, "OPENING DEPOSIT");
    draft = 150000;
    add_built(yymmdd_of(2016, 9, 16), "10004201", 'C', 80000, "OPENING DEPOSIT");
    share = 80000;

    while (yymmdd_of(y, m, d) <= end && np < 400) {
        pays[np++] = yymmdd_of(y, m, d);
        add_days(&y, &m, &d, 14);
    }

    add_built(yymmdd_of(2016, 10, 3), "10004204", 'C', 1850000, "LOAN ADVANCE");
    loan = 1850000;

    final_mv = retirement_mv();
    contrib_total = (long)N_MONTHS * (25000L + 10000L);
    div_total = final_mv - contrib_total;
    if (div_total <= 0) {
        fprintf(stderr, "retirement market value %ld does not cover contributions %ld\n",
                final_mv, contrib_total);
        exit(1);
    }
    div_each = div_total / N_MONTHS;

    for (i = 0; i < N_MONTHS; i++) {
        int yy;
        int mm;
        int last;
        int day;
        month_of(i, &yy, &mm);
        last = month_last_day(yy, mm);
        for (day = 1; day <= last; day++) {
            int date = yymmdd_of(yy, mm, day);
            char desc[32];
            if (is_payroll(date, pays, np)) {
                long pay = 215000 + rnd(70) * 100;
                long split = 8000 + rnd(40) * 100;
                sprintf(desc, "PAYROLL %02d%02d%02d", yy % 100, mm, day);
                add_built(date, "10004202", 'C', pay, desc);
                draft += pay;
                add_built(date, "10004201", 'C', split, "PAYROLL SPLIT");
                share += split;
            }
            if (day == 2) {
                sprintf(desc, "CHECK %04d RENT", checkno++);
                debit_draft(&draft, date, 158000 + rnd(140) * 100, desc);
            } else if (day == 4) {
                sprintf(desc, "APS ELECTRIC %02d%02d", yy % 100, mm);
                debit_draft(&draft, date, 8500 + rnd(90) * 100, desc);
            } else if (day == 5) {
                card_draft(&draft, date, groc[rnd(6)], 4200, 16000, &card);
            } else if (day == 6) {
                sprintf(desc, "ATM MESA %02d%02d", yy % 100, mm);
                debit_draft(&draft, date, 4000 + rnd(16) * 1000, desc);
            } else if (day == 7) {
                card_draft(&draft, date, fuel[rnd(5)], 3200, 7800, &card);
            } else if (day == 8) {
                sprintf(desc, "VERIZON %02d%02d", yy % 100, mm);
                debit_draft(&draft, date, 7200 + rnd(50) * 100, desc);
            } else if (day == 9) {
                card_draft(&draft, date, eat[rnd(4)], 800, 4200, &card);
            } else if (day == 10) {
                if ((i % 2) == 0) {
                    sprintf(desc, "GEICO INS %02d%02d", yy % 100, mm);
                } else {
                    sprintf(desc, "STATE FARM %02d%02d", yy % 100, mm);
                }
                debit_draft(&draft, date, 11000 + rnd(70) * 100, desc);
            } else if (day == 11) {
                card_draft(&draft, date, groc[rnd(6)], 3800, 14500, &card);
            } else if (day == 13) {
                card_draft(&draft, date, shop[rnd(7)], 1500, 9600, &card);
            } else if (day == 14 && (i % 3) == 0) {
                const char* fees[] = {"SVC FEE", "PAPER STMT", "ATM SURCH", "STOP PAY"};
                sprintf(desc, "%s %02d%02d", fees[rnd(4)], yy % 100, mm);
                debit_draft(&draft, date, 500 + rnd(30) * 100, desc);
            } else if (day == 15) {
                card_draft(&draft, date, groc[rnd(6)], 5100, 17000, &card);
                add_built(date, "10004204", 'D', 12500, "LOAN PAYMENT");
                loan -= 12500;
                add_built(date, "10004203", 'C', 25000, "EMPLOYEE CONTRIB");
                add_built(date, "10004203", 'C', 10000, "EMPLOYER MATCH");
                {
                    long divc = div_each;
                    if (i == N_MONTHS - 1) divc = div_total - div_used;
                    div_used += divc;
                    if (divc > 0) add_built(date, "10004203", 'C', divc, "DIVIDEND REINVEST");
                }
            } else if (day == 16) {
                sprintf(desc, "ATM 7TH %02d%02d", yy % 100, mm);
                debit_draft(&draft, date, 2000 + rnd(12) * 1000, desc);
            } else if (day == 18) {
                card_draft(&draft, date, fuel[rnd(5)], 2800, 6900, &card);
            } else if (day == 19) {
                card_draft(&draft, date, eat[rnd(4)], 1200, 5400, &card);
            } else if (day == 20) {
                if (share > 25000) {
                    long w = 6000 + rnd(50) * 100;
                    if (w < share - 5000) {
                        add_built(date, "10004201", 'D', w, "ATM WITHDRAWAL");
                        share -= w;
                    }
                }
            } else if (day == 22) {
                card_draft(&draft, date, shop[rnd(7)], 2000, 12000, &card);
            } else if (day == 24) {
                card_draft(&draft, date, groc[rnd(6)], 3600, 13000, &card);
            } else if (day == 27) {
                sprintf(desc, "CHECK %04d", checkno++);
                debit_draft(&draft, date, 25000 + rnd(200) * 100, desc);
            }
            if (day == last && draft > 450000) {
                long extra = draft - 220000;
                sprintf(desc, "CHECK %04d", checkno++);
                debit_draft(&draft, date, extra, desc);
            }
        }
        if (mm == 3 || mm == 6 || mm == 9 || mm == 12) {
            int divday = last < 28 ? last : 28;
            long divc;
            if (yy == 2026 && mm == 9) divday = 15;
            divc = share / 500;
            if (divc > 0) {
                add_built(yymmdd_of(yy, mm, divday), "10004201", 'C', divc, "QTRLY DIVIDEND");
                share += divc;
            }
        }
    }

    if (loan <= 0 || draft <= 0 || share <= 0) {
        fprintf(stderr, "bad ending balances draft %ld share %ld loan %ld\n", draft, share, loan);
        exit(1);
    }
    printf("simulated draft %ld share %ld loan %ld retire %ld\n", draft, share, loan, final_mv);
}

static long net_of(const char* acct) {
    long n = 0;
    int i;
    for (i = 0; i < g_ntx; i++) {
        const char* a;
        char dc;
        long cents;
        if (g_tx[i].keep) {
            a = g_tx[i].raw + TXN_ACCT_OFF;
            dc = g_tx[i].raw[TXN_DC_OFF];
            cents = field_amount(g_tx[i].raw, TXN_AMT_OFF, TXN_AMT_LEN);
        } else {
            a = g_tx[i].acct;
            dc = g_tx[i].dc;
            cents = g_tx[i].cents;
        }
        if (memcmp(a, acct, 8) != 0) continue;
        if (dc == 'C') n += cents;
        else n -= cents;
    }
    return n;
}

static void write_txns(void) {
    FILE* f;
    int i;
    qsort(g_tx, g_ntx, sizeof g_tx[0], cmp_txn);
    f = fopen(PATH_TXNS, "w");
    if (!f) exit(1);
    for (i = 0; i < g_ntx; i++) {
        char rec[TXN_LEN + 1];
        memset(rec, ' ', TXN_LEN);
        rec[TXN_LEN] = 0;
        if (g_tx[i].keep) memcpy(rec, g_tx[i].raw, TXN_LEN);
        else {
            char dateb[8];
            sprintf(dateb, "%06d", g_tx[i].date);
            put_text(rec, TXN_ACCT_OFF, TXN_ACCT_LEN, g_tx[i].acct);
            put_text(rec, TXN_DATE_OFF, TXN_DATE_LEN, dateb);
            rec[TXN_DC_OFF] = g_tx[i].dc;
            put_num(rec, TXN_AMT_OFF, TXN_AMT_LEN, g_tx[i].cents);
            put_text(rec, TXN_DESC_OFF, TXN_DESC_LEN, g_tx[i].desc);
        }
        put_num(rec, TXN_SEQ_OFF, TXN_SEQ_LEN, i + 1);
        if (fwrite(rec, 1, TXN_LEN, f) != (size_t)TXN_LEN || fputc('\n', f) == EOF) exit(1);
    }
    fclose(f);
}

static void write_acct_line(FILE* f, const char* num, const char* mbr, char type,
                            long cents, const char* open_d, char status) {
    char rec[ACCT_LEN + 1];
    memset(rec, ' ', ACCT_LEN);
    rec[ACCT_LEN] = 0;
    put_text(rec, ACCT_NUM_OFF, ACCT_NUM_LEN, num);
    put_text(rec, ACCT_MBR_OFF, ACCT_MBR_LEN, mbr);
    rec[ACCT_TYPE_OFF] = type;
    put_num(rec, ACCT_BAL_OFF, ACCT_BAL_LEN, cents);
    put_text(rec, ACCT_OPEN_OFF, ACCT_OPEN_LEN, open_d);
    rec[ACCT_STAT_OFF] = status;
    if (fwrite(rec, 1, ACCT_LEN, f) != (size_t)ACCT_LEN || fputc('\n', f) == EOF) exit(1);
}

static void write_accounts(void) {
    FILE* in = fopen(PATH_ACCOUNTS, "r");
    FILE* out;
    char line[256];
    char kept[16][ACCT_LEN + 1];
    int nk = 0;
    int i;
    long draft;
    long share;
    long retire;
    long loan;
    if (!in) exit(1);
    while (fgets(line, (int)sizeof line, in)) {
        int L = (int)strlen(line);
        while (L > 0 && (line[L - 1] == '\n' || line[L - 1] == '\r')) line[--L] = 0;
        if (L == 0) continue;
        if (L != ACCT_LEN) {
            fprintf(stderr, "account length %d\n", L);
            exit(1);
        }
        if (memcmp(line, "100042", 6) == 0) continue;
        if (nk >= 16) exit(1);
        memcpy(kept[nk], line, ACCT_LEN);
        kept[nk][ACCT_LEN] = 0;
        nk++;
    }
    fclose(in);
    draft = net_of("10004202");
    share = net_of("10004201");
    retire = net_of("10004203");
    loan = net_of("10004204");
    if (retire != retirement_mv()) {
        fprintf(stderr, "retirement ledger %ld market %ld\n", retire, retirement_mv());
        exit(1);
    }
    out = fopen(PATH_ACCOUNTS, "w");
    if (!out) exit(1);
    write_acct_line(out, "10004201", "100042", 'S', share, "160915", 'A');
    write_acct_line(out, "10004202", "100042", 'D', draft, "160915", 'A');
    write_acct_line(out, "10004203", "100042", 'R', retire, "161015", 'A');
    write_acct_line(out, "10004204", "100042", 'L', loan, "161003", 'A');
    for (i = 0; i < nk; i++) {
        if (fwrite(kept[i], 1, ACCT_LEN, out) != (size_t)ACCT_LEN || fputc('\n', out) == EOF) exit(1);
    }
    fclose(out);
    printf("balances share %ld draft %ld retire %ld loan %ld\n", share, draft, retire, loan);
}

static void write_funds_and_hist(void) {
    FILE* funds;
    FILE* pos;
    FILE* nav;
    FILE* val;
    int f;
    int i;
    int vfiax = 0;
    funds = fopen(PATH_FUNDS, "w");
    pos = fopen(PATH_POSITIONS, "w");
    nav = fopen(PATH_NAVHIST, "w");
    val = fopen(PATH_VALUEHIST, "w");
    if (!funds || !pos || !nav || !val) exit(1);
    for (f = 0; f < N_FUNDS; f++) {
        char rec[80];
        int navc = nav_at[f][N_MONTHS - 1];
        long shares = (long)fund_whole[f] * (long)POS_SHARES_SCALE;
        memset(rec, ' ', FUND_LEN);
        put_text(rec, FUND_ID_OFF, FUND_ID_LEN, FUND_ID[f]);
        put_text(rec, FUND_NAME_OFF, FUND_NAME_LEN, FUND_NAME[f]);
        put_num(rec, FUND_NAV_OFF, FUND_NAV_LEN, navc);
        put_text(rec, FUND_ASOF_OFF, FUND_ASOF_LEN, "260915");
        if (fwrite(rec, 1, FUND_LEN, funds) != (size_t)FUND_LEN || fputc('\n', funds) == EOF) exit(1);
        memset(rec, ' ', POS_LEN);
        put_text(rec, POS_ACCT_OFF, POS_ACCT_LEN, "10004203");
        put_text(rec, POS_FUND_OFF, POS_FUND_LEN, FUND_ID[f]);
        put_num(rec, POS_SHARES_OFF, POS_SHARES_LEN, shares);
        if (fwrite(rec, 1, POS_LEN, pos) != (size_t)POS_LEN || fputc('\n', pos) == EOF) exit(1);
    }
    for (i = 0; i < N_MONTHS; i++) {
        int yy;
        int mm;
        char dateb[8];
        month_of(i, &yy, &mm);
        sprintf(dateb, "%06d", yymmdd_of(yy, mm, 15));
        for (f = 0; f < N_FUNDS; f++) {
            char rec[80];
            long shares = (long)fund_whole[f] * (long)POS_SHARES_SCALE * (i + 1) / N_MONTHS;
            long mv = market_cents(shares, nav_at[f][i]);
            if (i == N_MONTHS - 1) {
                if (shares != (long)fund_whole[f] * (long)POS_SHARES_SCALE) {
                    fprintf(stderr, "final shares drifted\n");
                    exit(1);
                }
                if (mv != fund_mv[f]) {
                    fprintf(stderr, "final mv mismatch %s %ld %ld\n", FUND_ID[f], mv, fund_mv[f]);
                    exit(1);
                }
                if (nav_at[f][i] != nav_at[f][N_MONTHS - 1]) exit(1);
            }
            memset(rec, ' ', NAVH_LEN);
            put_text(rec, NAVH_FUND_OFF, NAVH_FUND_LEN, FUND_ID[f]);
            put_text(rec, NAVH_DATE_OFF, NAVH_DATE_LEN, dateb);
            put_num(rec, NAVH_NAV_OFF, NAVH_NAV_LEN, nav_at[f][i]);
            if (fwrite(rec, 1, NAVH_LEN, nav) != (size_t)NAVH_LEN || fputc('\n', nav) == EOF) exit(1);
            memset(rec, ' ', VALH_LEN);
            put_text(rec, VALH_ACCT_OFF, VALH_ACCT_LEN, "10004203");
            put_text(rec, VALH_FUND_OFF, VALH_FUND_LEN, FUND_ID[f]);
            put_text(rec, VALH_DATE_OFF, VALH_DATE_LEN, dateb);
            put_num(rec, VALH_SHARES_OFF, VALH_SHARES_LEN, shares);
            put_num(rec, VALH_VALUE_OFF, VALH_VALUE_LEN, mv);
            if (fwrite(rec, 1, VALH_LEN, val) != (size_t)VALH_LEN || fputc('\n', val) == EOF) exit(1);
            if (f == 0) vfiax++;
        }
    }
    fclose(funds);
    fclose(pos);
    fclose(nav);
    fclose(val);
    if (vfiax != 120) {
        fprintf(stderr, "vfiax rows %d\n", vfiax);
        exit(1);
    }
}

static void write_member(FILE* f, const char* num, const char* name, const char* ssn,
                         const char* dob, const char* addr, const char* phone,
                         char status, const char* pin, const char* email) {
    char rec[MBR_LEN + 1];
    memset(rec, ' ', MBR_LEN);
    rec[MBR_LEN] = 0;
    put_text(rec, MBR_NUM_OFF, MBR_NUM_LEN, num);
    put_text(rec, MBR_NAME_OFF, MBR_NAME_LEN, name);
    put_text(rec, MBR_SSN_OFF, MBR_SSN_LEN, ssn);
    put_text(rec, MBR_DOB_OFF, MBR_DOB_LEN, dob);
    put_text(rec, MBR_ADDR_OFF, MBR_ADDR_LEN, addr);
    put_text(rec, MBR_PHONE_OFF, MBR_PHONE_LEN, phone);
    rec[MBR_STAT_OFF] = status;
    put_text(rec, MBR_PIN_OFF, MBR_PIN_LEN, pin);
    put_text(rec, MBR_EMAIL_OFF, MBR_EMAIL_LEN, email);
    if (fwrite(rec, 1, MBR_LEN, f) != (size_t)MBR_LEN || fputc('\n', f) == EOF) exit(1);
}

static void write_members(void) {
    FILE* f = fopen(PATH_MEMBERS, "w");
    if (!f) exit(1);
    write_member(f, "100042", "ELENA VASQUEZ", "900-42-1001", "740315",
                 "220 CEDAR LANE, MESA AZ 85201", "480-555-0142", 'A', "2468",
                 "elena.vasquez@example.com");
    write_member(f, "100108", "JAMES OKONKWO", "900-18-2204", "680922",
                 "14 MAPLE STREET, DAYTON OH 45402", "937-555-0148", 'A', "1357",
                 "james.okonkwo@example.com");
    write_member(f, "100215", "PRIYA SHAH", "900-55-0198", "820704",
                 "88 RIVER ROAD, AUSTIN TX 78701", "512-555-0198", 'A', "8021",
                 "priya.shah@example.com");
    write_member(f, "100330", "ROBERT CHEN", "900-33-7740", "590211",
                 "402 OAK AVE, PORTLAND OR 97201", "503-555-0177", 'A', "4410",
                 "robert.chen@example.com");
    write_member(f, "100417", "MARIA ALVAREZ", "900-61-3088", "710528",
                 "9 PINE COURT, TUCSON AZ 85701", "520-555-0164", 'F', "9900",
                 "maria.alvarez@example.com");
    fclose(f);
}

static void write_payees(void) {
    FILE* f = fopen(PATH_PAYEES, "w");
    const char* ids[] = {"ELEC", "PHON", "INSU", "CARD"};
    const char* names[] = {"ELECTRIC", "PHONE", "INSURANCE", "CREDIT CARD"};
    const char* descs[] = {"ELECTRIC BILL", "PHONE BILL", "INSURANCE PREMIUM", "CREDIT CARD PAYMENT"};
    int i;
    if (!f) exit(1);
    for (i = 0; i < 4; i++) {
        char rec[PAYEE_LEN + 1];
        memset(rec, ' ', PAYEE_LEN);
        rec[PAYEE_LEN] = 0;
        put_text(rec, PAYEE_ID_OFF, PAYEE_ID_LEN, ids[i]);
        put_text(rec, PAYEE_MBR_OFF, PAYEE_MBR_LEN, "100042");
        put_text(rec, PAYEE_NAME_OFF, PAYEE_NAME_LEN, names[i]);
        put_text(rec, PAYEE_DESC_OFF, PAYEE_DESC_LEN, descs[i]);
        if (fwrite(rec, 1, PAYEE_LEN, f) != (size_t)PAYEE_LEN || fputc('\n', f) == EOF) exit(1);
    }
    fclose(f);
}

static int count_acct(const char* acct) {
    int n = 0;
    int i;
    for (i = 0; i < g_ntx; i++) {
        const char* a = g_tx[i].keep ? g_tx[i].raw + TXN_ACCT_OFF : g_tx[i].acct;
        if (memcmp(a, acct, 8) == 0) n++;
    }
    return n;
}

static int count_unique_draft(void) {
    int n = 0;
    int i;
    int j;
    for (i = 0; i < g_ntx; i++) {
        char desc[25];
        int seen = 0;
        if (g_tx[i].keep) continue;
        if (memcmp(g_tx[i].acct, "10004202", 8) != 0) continue;
        memset(desc, 0, sizeof desc);
        strncpy(desc, g_tx[i].desc, 24);
        for (j = 0; j < i; j++) {
            if (g_tx[j].keep) continue;
            if (memcmp(g_tx[j].acct, "10004202", 8) != 0) continue;
            if (strncmp(g_tx[j].desc, desc, 24) == 0) {
                seen = 1;
                break;
            }
        }
        if (!seen) n++;
    }
    return n;
}

int main(void) {
    int draft_n;
    int uniq;
    build_navs();
    choose_shares();
    load_kept_txns();
    build_elena_activity();
    draft_n = count_acct("10004202");
    uniq = count_unique_draft();
    printf("txns %d draft %d unique_draft_desc %d\n", g_ntx, draft_n, uniq);
    if (draft_n < 1500) {
        fprintf(stderr, "draft history too short\n");
        exit(1);
    }
    if (uniq < 400) {
        fprintf(stderr, "draft descriptions are too repetitive\n");
        exit(1);
    }
    write_txns();
    write_accounts();
    write_funds_and_hist();
    write_members();
    write_payees();
    printf("seedhist ok\n");
    return 0;
}
