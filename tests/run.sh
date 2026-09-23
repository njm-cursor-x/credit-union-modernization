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

shares_of() {
    awk -v a="$1" -v f="$2" 'substr($0,1,8)==a && substr($0,9,5)==f { print substr($0,14,11)+0; exit }' "$TMP/data/positions.dat"
}

nav_of() {
    awk -v f="$1" 'substr($0,1,5)==f { print substr($0,54,11)+0; exit }' "$TMP/data/funds.dat"
}

numbal() {
    awk -v a="$1" 'BEGIN { print a + 0 }'
}

# One posted invest buys shares at the stored NAV and changes positions.dat.
vf_before=$(shares_of 10004203 VFIAX)
draft_before=$(numbal "$(bal 10004202)")
ret_before=$(numbal "$(bal 10004203)")
awk 'BEGIN {
    rec = sprintf("%-1s%-4s%-3s%-8s%-8s%-6s%011d%-24s",
        "W", "0000", "INV", "10004202", "10004203", "260923", 10000, "VFIAX")
    if (length(rec) != 65) {
        printf "invest pending length %d\n", length(rec) > "/dev/stderr"
        exit 1
    }
    print rec
}' > "$TMP/data/pending.dat"
"$ROOT/bin/postpend" > "$TMP/post3.out"
vf_nav=$(nav_of VFIAX)
vf_buy=$(awk -v nav="$vf_nav" 'BEGIN { print int(10000 * 10000 / nav) }')
vf_after=$(shares_of 10004203 VFIAX)
vf_expect=$(awk -v b="$vf_before" -v n="$vf_buy" 'BEGIN { print b + n }')
if [ "$vf_after" -ne "$vf_expect" ]; then
    echo "invest did not buy VFIAX shares: $vf_before + $vf_buy -> $vf_after" >&2
    cat "$TMP/post3.out" >&2
    exit 1
fi
draft_after=$(numbal "$(bal 10004202)")
ret_after=$(numbal "$(bal 10004203)")
if [ "$draft_after" -ne $((draft_before - 10000)) ]; then
    echo "invest did not reduce the draft: $draft_before -> $draft_after" >&2
    exit 1
fi
if [ "$ret_after" -ne $((ret_before + 10000)) ]; then
    echo "invest did not credit retirement: $ret_before -> $ret_after" >&2
    exit 1
fi
if ! grep -q "INVEST VFIAX" "$TMP/data/txns.dat"; then
    echo "posted invest description missing" >&2
    exit 1
fi
if cmp -s "$ROOT/data/positions.dat" "$TMP/data/positions.dat"; then
    echo "posted invest left positions.dat unchanged" >&2
    exit 1
fi
recon 10004202
recon 10004203

# A posted exchange moves shares and does not take cash out of retirement.
vm_before=$(shares_of 10004203 VMFXX)
vb_before=$(shares_of 10004203 VBTLX)
ret_held=$ret_after
draft_held=$draft_after
awk 'BEGIN {
    rec = sprintf("%-1s%-4s%-3s%-8s%-8s%-6s%011d%-24s",
        "W", "0000", "XCH", "10004203", "", "260923", 5000, "VMFXXVBTLX")
    if (length(rec) != 65) {
        printf "exchange pending length %d\n", length(rec) > "/dev/stderr"
        exit 1
    }
    print rec
}' > "$TMP/data/pending.dat"
"$ROOT/bin/postpend" > "$TMP/post4.out"
vm_nav=$(nav_of VMFXX)
vb_nav=$(nav_of VBTLX)
vm_sell=$(awk -v nav="$vm_nav" 'BEGIN { print int(5000 * 10000 / nav) }')
vb_buy=$(awk -v nav="$vb_nav" 'BEGIN { print int(5000 * 10000 / nav) }')
vm_after=$(shares_of 10004203 VMFXX)
vb_after=$(shares_of 10004203 VBTLX)
vm_expect=$(awk -v b="$vm_before" -v n="$vm_sell" 'BEGIN { print b - n }')
vb_expect=$(awk -v b="$vb_before" -v n="$vb_buy" 'BEGIN { print b + n }')
if [ "$vm_after" -ne "$vm_expect" ] || [ "$vb_after" -ne "$vb_expect" ]; then
    echo "exchange shares VMFXX $vm_before->$vm_after VBTLX $vb_before->$vb_after" >&2
    cat "$TMP/post4.out" >&2
    exit 1
fi
if [ "$(numbal "$(bal 10004203)")" -ne "$ret_held" ]; then
    echo "exchange moved retirement cash" >&2
    exit 1
fi
if [ "$(numbal "$(bal 10004202)")" -ne "$draft_held" ]; then
    echo "exchange moved the draft" >&2
    exit 1
fi
if ! grep -q "EXCH VMFXX TO VBTLX" "$TMP/data/txns.dat"; then
    echo "posted exchange description missing" >&2
    exit 1
fi
recon 10004203

# An exchange the position cannot cover is rejected and leaves shares alone.
cp "$TMP/data/positions.dat" "$TMP/positions.snap"
awk 'BEGIN {
    rec = sprintf("%-1s%-4s%-3s%-8s%-8s%-6s%011d%-24s",
        "W", "0000", "XCH", "10004203", "", "260923", 10000000, "VMFXXVTIAX")
    if (length(rec) != 65) {
        printf "reject pending length %d\n", length(rec) > "/dev/stderr"
        exit 1
    }
    print rec
}' > "$TMP/data/pending.dat"
"$ROOT/bin/postpend" > "$TMP/post5.out"
if ! cmp -s "$TMP/positions.snap" "$TMP/data/positions.dat"; then
    echo "rejected exchange changed positions.dat" >&2
    exit 1
fi
if [ "$(numbal "$(bal 10004203)")" -ne "$ret_held" ]; then
    echo "rejected exchange changed retirement cash" >&2
    exit 1
fi
if ! grep -q "10004203" "$TMP/data/reject.dat"; then
    echo "reject.dat has no exchange line" >&2
    cat "$TMP/data/reject.dat" >&2
    exit 1
fi
if ! grep -q "INSUFFICIENT SHARES" "$TMP/data/reject.dat"; then
    echo "reject.dat missing share reason" >&2
    cat "$TMP/data/reject.dat" >&2
    exit 1
fi

echo "ok"
