/* Fixed-width ASCII layouts. Widths match the copybooks in src/cobol/copy/. */
#ifndef LCU_LAYOUT_H
#define LCU_LAYOUT_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
    MBR_NUM_OFF = 0, MBR_NUM_LEN = 6,
    MBR_NAME_OFF = 6, MBR_NAME_LEN = 30,
    MBR_SSN_OFF = 36, MBR_SSN_LEN = 11,
    MBR_DOB_OFF = 47, MBR_DOB_LEN = 6,
    MBR_ADDR_OFF = 53, MBR_ADDR_LEN = 40,
    MBR_PHONE_OFF = 93, MBR_PHONE_LEN = 12,
    MBR_STAT_OFF = 105, MBR_STAT_LEN = 1,
    MBR_PIN_OFF = 106, MBR_PIN_LEN = 4,
    MBR_EMAIL_OFF = 110, MBR_EMAIL_LEN = 40,
    MBR_LEN = 150
};

enum {
    ACCT_NUM_OFF = 0, ACCT_NUM_LEN = 8,
    ACCT_MBR_OFF = 8, ACCT_MBR_LEN = 6,
    ACCT_TYPE_OFF = 14, ACCT_TYPE_LEN = 1,
    ACCT_BAL_OFF = 15, ACCT_BAL_LEN = 11,
    ACCT_OPEN_OFF = 26, ACCT_OPEN_LEN = 6,
    ACCT_STAT_OFF = 32, ACCT_STAT_LEN = 1,
    ACCT_LEN = 33
};

enum {
    TXN_SEQ_OFF = 0, TXN_SEQ_LEN = 6,
    TXN_ACCT_OFF = 6, TXN_ACCT_LEN = 8,
    TXN_DATE_OFF = 14, TXN_DATE_LEN = 6,
    TXN_DC_OFF = 20, TXN_DC_LEN = 1,
    TXN_AMT_OFF = 21, TXN_AMT_LEN = 11,
    TXN_DESC_OFF = 32, TXN_DESC_LEN = 24,
    TXN_LEN = 56
};

enum {
    PEND_SRC_OFF = 0, PEND_SRC_LEN = 1,
    PEND_TELLER_OFF = 1, PEND_TELLER_LEN = 4,
    PEND_CODE_OFF = 5, PEND_CODE_LEN = 3,
    PEND_FROM_OFF = 8, PEND_FROM_LEN = 8,
    PEND_TO_OFF = 16, PEND_TO_LEN = 8,
    PEND_DATE_OFF = 24, PEND_DATE_LEN = 6,
    PEND_AMT_OFF = 30, PEND_AMT_LEN = 11,
    PEND_DESC_OFF = 41, PEND_DESC_LEN = 24,
    PEND_LEN = 65
};

enum {
    TLR_ID_OFF = 0, TLR_ID_LEN = 4,
    TLR_NAME_OFF = 4, TLR_NAME_LEN = 20,
    TLR_PASS_OFF = 24, TLR_PASS_LEN = 8,
    TLR_LEN = 32
};

enum { CTRL_LEN = 12 };

enum {
    FUND_ID_OFF = 0, FUND_ID_LEN = 5,
    FUND_NAME_OFF = 5, FUND_NAME_LEN = 48,
    FUND_NAV_OFF = 53, FUND_NAV_LEN = 11,
    FUND_ASOF_OFF = 64, FUND_ASOF_LEN = 6,
    FUND_LEN = 70
};

enum {
    POS_ACCT_OFF = 0, POS_ACCT_LEN = 8,
    POS_FUND_OFF = 8, POS_FUND_LEN = 5,
    POS_SHARES_OFF = 13, POS_SHARES_LEN = 11,
    POS_LEN = 24,
    /* shares are 9(7)V9(4): raw integer / POS_SHARES_SCALE */
    POS_SHARES_SCALE = 10000
};

enum {
    NAVH_FUND_OFF = 0, NAVH_FUND_LEN = 5,
    NAVH_DATE_OFF = 5, NAVH_DATE_LEN = 6,
    NAVH_NAV_OFF = 11, NAVH_NAV_LEN = 11,
    NAVH_LEN = 22
};

enum {
    VALH_ACCT_OFF = 0, VALH_ACCT_LEN = 8,
    VALH_FUND_OFF = 8, VALH_FUND_LEN = 5,
    VALH_DATE_OFF = 13, VALH_DATE_LEN = 6,
    VALH_SHARES_OFF = 19, VALH_SHARES_LEN = 11,
    VALH_VALUE_OFF = 30, VALH_VALUE_LEN = 11,
    VALH_LEN = 41
};

enum {
    PAYEE_ID_OFF = 0, PAYEE_ID_LEN = 4,
    PAYEE_MBR_OFF = 4, PAYEE_MBR_LEN = 6,
    PAYEE_NAME_OFF = 10, PAYEE_NAME_LEN = 20,
    PAYEE_DESC_OFF = 30, PAYEE_DESC_LEN = 24,
    PAYEE_LEN = 54
};

enum { REJ_LEN = 105, REJ_REASON_LEN = 40 };

static const char PATH_MEMBERS[] = "data/members.dat";
static const char PATH_ACCOUNTS[] = "data/accounts.dat";
static const char PATH_TXNS[] = "data/txns.dat";
static const char PATH_PENDING[] = "data/pending.dat";
static const char PATH_TELLERS[] = "data/tellers.dat";
static const char PATH_CONTROL[] = "data/control.dat";
static const char PATH_REJECT[] = "data/reject.dat";
static const char PATH_FUNDS[] = "data/funds.dat";
static const char PATH_POSITIONS[] = "data/positions.dat";
static const char PATH_NAVHIST[] = "data/navhist.dat";
static const char PATH_VALUEHIST[] = "data/valuehist.dat";
static const char PATH_PAYEES[] = "data/payees.dat";

typedef char mbr_len_ok[(MBR_LEN == 150) ? 1 : -1];
typedef char acct_len_ok[(ACCT_LEN == 33) ? 1 : -1];
typedef char txn_len_ok[(TXN_LEN == 56) ? 1 : -1];
typedef char pend_len_ok[(PEND_LEN == 65) ? 1 : -1];
typedef char fund_len_ok[(FUND_LEN == 70) ? 1 : -1];
typedef char pos_len_ok[(POS_LEN == 24) ? 1 : -1];
typedef char navh_len_ok[(NAVH_LEN == 22) ? 1 : -1];
typedef char valh_len_ok[(VALH_LEN == 41) ? 1 : -1];
typedef char payee_len_ok[(PAYEE_LEN == 54) ? 1 : -1];

static void put_text(char* rec, int off, int len, const char* val) {
    int i;
    int n = 0;
    if (val) n = (int)strlen(val);
    if (n > len) n = len;
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)val[i];
        if (c < 32 || c > 126) c = ' ';
        rec[off + i] = (char)c;
    }
    for (; i < len; i++) rec[off + i] = ' ';
}

static void field_copy(const char* rec, int off, int len, char* dest) {
    int end = len;
    memcpy(dest, rec + off, len);
    dest[len] = 0;
    while (end > 0 && dest[end - 1] == ' ') dest[--end] = 0;
}

static int field_eq(const char* rec, int off, int len, const char* val) {
    int i;
    if (!val) return 0;
    for (i = 0; i < len; i++) {
        if (val[i] == 0) {
            for (; i < len; i++) {
                if (rec[off + i] != ' ') return 0;
            }
            return 1;
        }
        if (rec[off + i] != val[i]) return 0;
    }
    return val[len] == 0;
}

static long field_amount(const char* rec, int off, int len) {
    long v = 0;
    int i;
    for (i = 0; i < len; i++) {
        char c = rec[off + i];
        if (c >= '0' && c <= '9') v = v * 10 + (c - '0');
    }
    return v;
}

/* Dollars text ("10", "10.00") to implied-decimal cents. No binary float. */
static int dollars_to_cents(const char* s, long* out) {
    long whole = 0;
    long frac = 0;
    int digits = 0;
    int seen_dot = 0;
    const char* p = s;
    if (!s || !*s) return -1;
    if (*p == '+') p++;
    if (!isdigit((unsigned char)*p)) return -1;
    while (*p && *p != ' ') {
        if (*p == '.') {
            if (seen_dot) return -1;
            seen_dot = 1;
        } else if (isdigit((unsigned char)*p)) {
            if (!seen_dot) {
                whole = whole * 10 + (*p - '0');
            } else if (digits < 2) {
                frac = frac * 10 + (*p - '0');
                digits++;
            } else {
                return -1;
            }
        } else {
            return -1;
        }
        p++;
    }
    while (digits < 2) {
        frac *= 10;
        digits++;
    }
    *out = whole * 100 + frac;
    return 0;
}

static void format_money(char* dest, long cents) {
    char digits[32];
    char with[40];
    int neg = 0;
    int n;
    int i;
    int groups;
    int first;
    int w = 0;
    long dollars;
    int frac;
    if (cents < 0) {
        neg = 1;
        cents = -cents;
    }
    dollars = cents / 100;
    frac = (int)(cents % 100);
    sprintf(digits, "%ld", dollars);
    n = (int)strlen(digits);
    groups = (n - 1) / 3;
    first = n - groups * 3;
    if (neg) with[w++] = '-';
    for (i = 0; i < n; i++) {
        if (i == first && groups > 0) with[w++] = ',';
        if (i > first && (i - first) % 3 == 0) with[w++] = ',';
        with[w++] = digits[i];
    }
    with[w] = 0;
    sprintf(dest, "%s.%02d", with, frac);
}

static void format_date(const char* yymmdd, char* dest) __attribute__((unused));
static void format_date(const char* yymmdd, char* dest) {
    if (!yymmdd || strlen(yymmdd) < 6) {
        strcpy(dest, "");
        return;
    }
    dest[0] = yymmdd[2];
    dest[1] = yymmdd[3];
    dest[2] = '/';
    dest[3] = yymmdd[4];
    dest[4] = yymmdd[5];
    dest[5] = '/';
    dest[6] = yymmdd[0];
    dest[7] = yymmdd[1];
    dest[8] = 0;
}

static void format_stamp(const char* stamp, char* dest) __attribute__((unused));
static void format_stamp(const char* stamp, char* dest) {
    if (!stamp || strlen(stamp) < 12) {
        strcpy(dest, "unknown");
        return;
    }
    sprintf(dest, "%c%c/%c%c/%c%c %c%c:%c%c:%c%c",
            stamp[2], stamp[3], stamp[4], stamp[5], stamp[0], stamp[1],
            stamp[6], stamp[7], stamp[8], stamp[9], stamp[10], stamp[11]);
}

static void today_yymmdd(char out[7]) {
    char tmp[16];
    time_t t = time(0);
    struct tm* tm = localtime(&t);
    int y = 0;
    int m = 1;
    int d = 1;
    if (tm) {
        y = tm->tm_year % 100;
        m = tm->tm_mon + 1;
        d = tm->tm_mday;
        if (y < 0) y += 100;
        if (m < 1) m = 1;
        if (m > 12) m = 12;
        if (d < 1) d = 1;
        if (d > 31) d = 31;
    }
    sprintf(tmp, "%02d%02d%02d", y, m, d);
    memcpy(out, tmp, 6);
    out[6] = 0;
}

/* Short lines are space-padded. GnuCOBOL LINE SEQUENTIAL drops trailing spaces. */
static int read_records(const char* path, char* storage, int rec_len,
                        int stride, int maxn) {
    FILE* f = fopen(path, "r");
    char line[1024];
    int n = 0;
    if (!f) return -1;
    while (fgets(line, (int)sizeof line, f)) {
        int L = (int)strlen(line);
        while (L > 0 && (line[L - 1] == '\n' || line[L - 1] == '\r')) line[--L] = 0;
        if (L == 0) continue;
        if (L > rec_len || n >= maxn) {
            fclose(f);
            return -2;
        }
        memset(storage + n * stride, ' ', rec_len);
        memcpy(storage + n * stride, line, L);
        storage[n * stride + rec_len] = 0;
        n++;
    }
    fclose(f);
    return n;
}

static int append_pending(char src, const char* teller, const char* code,
                          const char* from_acct, const char* to_acct,
                          long cents, const char* desc) {
    char rec[PEND_LEN + 1];
    char date[7];
    char cents_buf[32];
    FILE* f;
    memset(rec, ' ', PEND_LEN);
    rec[PEND_LEN] = 0;
    rec[PEND_SRC_OFF] = src;
    put_text(rec, PEND_TELLER_OFF, PEND_TELLER_LEN, teller ? teller : "0000");
    put_text(rec, PEND_CODE_OFF, PEND_CODE_LEN, code);
    put_text(rec, PEND_FROM_OFF, PEND_FROM_LEN, from_acct);
    if (to_acct && to_acct[0]) put_text(rec, PEND_TO_OFF, PEND_TO_LEN, to_acct);
    today_yymmdd(date);
    put_text(rec, PEND_DATE_OFF, PEND_DATE_LEN, date);
    sprintf(cents_buf, "%011ld", cents);
    memcpy(rec + PEND_AMT_OFF, cents_buf, PEND_AMT_LEN);
    put_text(rec, PEND_DESC_OFF, PEND_DESC_LEN, desc ? desc : "");
    f = fopen(PATH_PENDING, "a");
    if (!f) return -1;
    if (fwrite(rec, 1, PEND_LEN, f) != (size_t)PEND_LEN || fputc('\n', f) == EOF) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

static const char* acct_type_name(char type) {
    if (type == 'S') return "Regular Share";
    if (type == 'D') return "Share Draft";
    if (type == 'L') return "Loan";
    if (type == 'R') return "Retirement";
    return "Account";
}

static const char* status_name(char st) __attribute__((unused));
static const char* status_name(char st) {
    if (st == 'A') return "Active";
    if (st == 'C') return "Closed";
    if (st == 'F') return "Frozen";
    return "Unknown";
}

#endif
