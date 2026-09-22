var bals;

function syncBal() {
    var f = document.forms["xfer"];
    var acct;
    if (!f || !f.fromacct) return;
    acct = f.fromacct.options[f.fromacct.selectedIndex].value;
    if (bals && bals[acct] != null) {
        f.shownbal.value = bals[acct];
    }
}

function checkTransfer() {
    var f = document.forms["xfer"];
    var raw;
    var cents;
    var bal;
    if (!f) return false;
    raw = f.amount.value;
    cents = Math.round(parseFloat(raw) * 100);
    bal = parseInt(f.shownbal.value, 10);
    if (isNaN(cents) || cents <= 0) {
        alert("Enter a dollar amount greater than zero.");
        return false;
    }
    if (f.fromacct.value == f.toacct.value) {
        alert("Choose two different accounts.");
        return false;
    }
    if (isNaN(bal) || cents > bal) {
        alert("That amount is more than the balance shown for this account. The transfer was not sent.");
        return false;
    }
    return true;
}
