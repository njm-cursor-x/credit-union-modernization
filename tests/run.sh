#!/bin/sh
# Copy the ledger, post through the teller, and prove the batch is what moves money.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

if [ ! -x "$ROOT/bin/teller" ] || [ ! -x "$ROOT/bin/postpend" ]; then
    echo "build first: make" >&2
    exit 1
fi

cp -a "$ROOT/data" "$TMP/data"
: > "$TMP/data/reject.dat"

check_len() {
    local file="$1"
    local expect="$2"
    awk -v n="$expect" 'length($0) != n { printf "%s line %d length %d\n", FILENAME, NR, length($0); bad = 1 } END { exit bad }' "$file"
}

check_len "$TMP/data/members.dat" 150
check_len "$TMP/data/accounts.dat" 33
check_len "$TMP/data/txns.dat" 56
check_len "$TMP/data/tellers.dat" 32
check_len "$TMP/data/funds.dat" 70
check_len "$TMP/data/positions.dat" 24
check_len "$TMP/data/navhist.dat" 22
check_len "$TMP/data/valuehist.dat" 41
check_len "$TMP/data/payees.dat" 54
check_len "$TMP/data/control.dat" 12

pin=$(awk 'substr($0,1,6)=="100042" { print substr($0,107,4); exit }' "$TMP/data/members.dat")
if [ "$pin" != "2468" ]; then
    echo "Elena PIN field is '$pin'" >&2
    exit 1
fi

bal() {
    awk -v a="$1" 'substr($0,1,8)==a { print substr($0,16,11); exit }' "$TMP/data/accounts.dat"
}

before=$(bal 10004202)
if [ -z "$before" ]; then
    echo "missing Elena draft account" >&2
    exit 1
fi

cd "$TMP"
{
    echo "LOGIN 0001 WESTON01"
    echo "DEP 10004202 10.00 TELLER DEPOSIT"
} | "$ROOT/bin/teller" > "$TMP/teller1.out"

if ! grep -q "OK DEP 10004202 00000001000" "$TMP/teller1.out"; then
    echo "teller did not accept the deposit" >&2
    cat "$TMP/teller1.out" >&2
    exit 1
fi

mid=$(bal 10004202)
if [ "$mid" != "$before" ]; then
    echo "draft balance changed before batch: $before -> $mid" >&2
    exit 1
fi

if ! awk 'length($0)==65 { found=1 } END { exit !found }' "$TMP/data/pending.dat"; then
    echo "pending deposit was not 65 columns" >&2
    cat "$TMP/data/pending.dat" >&2
    exit 1
fi

"$ROOT/bin/postpend" > "$TMP/post1.out"

after=$(bal 10004202)
expect=$(awk -v b="$before" 'BEGIN { print b + 1000 }')
got=$(awk -v a="$after" 'BEGIN { print a + 0 }')
if [ "$got" -ne "$expect" ]; then
    echo "batch did not post 10.00: before $before after $after" >&2
    cat "$TMP/post1.out" >&2
    exit 1
fi

cents=$(awk -v a="$after" 'BEGIN { print a + 4000 }')
whole=$((cents / 100))
frac=$((cents % 100))
if [ "$frac" -lt 10 ]; then
    amt="${whole}.0${frac}"
else
    amt="${whole}.${frac}"
fi
{
    echo "LOGIN 0001 WESTON01"
    echo "WDL 10004202 $amt COURTESY CASH"
} | "$ROOT/bin/teller" > "$TMP/teller2.out"

if ! grep -q "COURTESY" "$TMP/teller2.out"; then
    echo "teller did not allow the $50 courtesy overdraft" >&2
    cat "$TMP/teller2.out" >&2
    exit 1
fi

held=$(bal 10004202)
if [ "$held" != "$after" ]; then
    echo "courtesy withdrawal changed the balance early" >&2
    exit 1
fi

"$ROOT/bin/postpend" > "$TMP/post2.out"
final=$(bal 10004202)
if [ "$final" != "$after" ]; then
    echo "batch posted an overdraft: $after -> $final" >&2
    cat "$TMP/post2.out" >&2
    exit 1
fi

if ! grep -q "10004202" "$TMP/data/reject.dat"; then
    echo "reject.dat has no overdraft line" >&2
    cat "$TMP/data/reject.dat" >&2
    exit 1
fi

if ! grep -q "NSF" "$TMP/data/reject.dat"; then
    echo "reject.dat missing NSF reason" >&2
    cat "$TMP/data/reject.dat" >&2
    exit 1
fi

pos=$(awk 'substr($0,1,8)=="10004203" { print substr($0,9,5) }' "$TMP/data/positions.dat" | sort | tr '\n' ' ')
if [ "$pos" != "VBTLX VFIAX VMFXX VTIAX " ]; then
    echo "Elena retirement positions are '$pos'" >&2
    exit 1
fi

funds=$(awk 'NF { n++ } END { print n+0 }' "$TMP/data/funds.dat")
if [ "$funds" -ne 4 ]; then
    echo "retirement file has $funds funds" >&2
    exit 1
fi

draft_n=$(awk 'substr($0,7,8)=="10004202" { n++ } END { print n+0 }' "$TMP/data/txns.dat")
if [ "$draft_n" -lt 1500 ]; then
    echo "Elena draft history has $draft_n rows" >&2
    exit 1
fi

vfiax=$(awk 'substr($0,1,5)=="VFIAX" { n++ } END { print n+0 }' "$TMP/data/navhist.dat")
if [ "$vfiax" -ne 120 ]; then
    echo "navhist VFIAX rows $vfiax" >&2
    exit 1
fi

recon() {
    acct="$1"
    net=$(awk -v a="$acct" 'substr($0,7,8)==a {
        amt = substr($0,22,11) + 0
        if (substr($0,21,1)=="C") c += amt
        else d += amt
    } END { print c - d }' "$TMP/data/txns.dat")
    got=$(bal "$acct")
    gotn=$(awk -v a="$got" 'BEGIN { print a + 0 }')
    if [ "$gotn" -ne "$net" ]; then
        echo "balance mismatch $acct file $gotn history $net" >&2
        exit 1
    fi
}
recon 10004201
recon 10004202
recon 10004203
recon 10004204

if ! cmp -s "$ROOT/data/positions.dat" "$TMP/data/positions.dat"; then
    echo "batch rewrote positions.dat" >&2
    exit 1
fi

ctrl=$(tr -d '\n' < "$TMP/data/control.dat")
if [ "${#ctrl}" -ne 12 ]; then
    echo "control stamp length ${#ctrl}" >&2
    exit 1
fi

echo "ok"
