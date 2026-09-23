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

function amountCents(raw) {
    var cents = Math.round(parseFloat(raw) * 100);
    return cents;
}

function checkTransfer() {
    var f = document.forms["xfer"];
    var cents;
    var bal;
    if (!f) return false;
    cents = amountCents(f.amount.value);
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

function checkDeposit() {
    var f = document.forms["dep"];
    var cents;
    if (!f) return false;
    cents = amountCents(f.amount.value);
    if (isNaN(cents) || cents <= 0) {
        alert("Enter a dollar amount greater than zero.");
        return false;
    }
    if (!f.acct || !f.acct.value) {
        alert("Choose an account.");
        return false;
    }
    return true;
}

function checkDebit(formName) {
    var f = document.forms[formName];
    var cents;
    var acct;
    var bal;
    if (!f) return false;
    cents = amountCents(f.amount.value);
    if (isNaN(cents) || cents <= 0) {
        alert("Enter a dollar amount greater than zero.");
        return false;
    }
    if (!f.acct || !f.acct.value) {
        alert("Choose an account.");
        return false;
    }
    acct = f.acct.options[f.acct.selectedIndex].value;
    bal = (bals && bals[acct] != null) ? bals[acct] : NaN;
    if (isNaN(bal) || cents > bal) {
        alert("That amount is more than the balance shown for this account. The transaction was not sent.");
        return false;
    }
    return true;
}

function checkProfile() {
    var f = document.forms["profile"];
    var at;
    if (!f) return false;
    if (!f.address.value || !f.phone.value) {
        alert("Enter a mailing address and a phone number.");
        return false;
    }
    at = f.email.value.indexOf("@");
    if (at < 1) {
        alert("Enter an email address.");
        return false;
    }
    return true;
}

function checkPin() {
    var f = document.forms["pinform"];
    var p;
    var i;
    if (!f) return false;
    p = f.newpin.value;
    if (p.length != 4) {
        alert("Enter a 4-digit PIN.");
        return false;
    }
    for (i = 0; i < 4; i++) {
        if (p.charAt(i) < "0" || p.charAt(i) > "9") {
            alert("Enter a 4-digit PIN.");
            return false;
        }
    }
    if (p != f.confirm.value) {
        alert("The new PIN and the confirmation do not match.");
        return false;
    }
    return true;
}
