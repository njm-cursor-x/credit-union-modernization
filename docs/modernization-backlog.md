# Modernization backlog

This is an inventory of what is old in the legacy credit union. Each item names the file and the behavior a later modernization would replace. Nothing in this list is fixed in the legacy baseline.

1. Plaintext PIN and SSN. `data/members.dat` stores the 4-digit PIN and the SSN in the clear. `src/cpp/webd.cpp` signs a member on by comparing that PIN. `src/cobol/stmt.cob` prints the SSN on the statement.

2. Stale balances until batch. The teller and the website append `data/pending.dat`. Balances in `data/accounts.dat` move only when `src/cobol/postpend.cob` runs. Refreshing the site before the batch still shows the previous nightly balance.

3. Teller and batch disagree on overdrafts. `src/cpp/teller.cpp` allows a $50 courtesy overdraft on a share draft withdrawal. `src/cobol/postpend.cob` refuses any debit that would take a balance below zero and writes the line to `data/reject.dat`.

4. No file locking. `src/cpp/teller.cpp`, `src/cpp/webd.cpp`, and the COBOL programs open the fixed-width files with no lock. Two writers can interleave a record.

5. Six-digit dates and magic codes. Dates are `YYMMDD` in `src/cpp/layout.h` and the copybooks under `src/cobol/copy/`. Status is one letter (`A` active, `C` closed, `F` frozen). Account type is one letter (`S` share, `D` draft, `L` loan, `R` retirement). Posted activity is `C` or `D`.

6. Dividend rate hardcoded in COBOL. `src/cobol/postdiv.cob` posts 0.15 percent for the month from a `WORKING-STORAGE` literal. There is no rate file. Retirement NAVs are not part of that job.

7. Retirement prices live in a flat file. `data/funds.dat` holds the fund id, name, NAV, and as-of date. Nothing reprices those rows. Someone at the credit union edits the file.

8. No online trading. The holdings page served by `src/cpp/webd.cpp` lists fund, shares, price, and market value, then tells the member to call the branch. The site cannot place a trade.

9. Contributions do not buy shares. `src/cobol/postpend.cob` increases the retirement cash balance when a transfer posts into a type `R` account. It does not update `data/positions.dat`, so the posted balance and the holdings total can diverge after a contribution.

10. Unsigned session cookie. `src/cpp/webd.cpp` stores the member number in the cookie `LCUMember` with no signature and no expiry. The browser that presents the cookie is that member.

11. Balance check only in the browser. `web/bank.js` compares the typed transfer amount with the balance shown on the form and can block the submit. The POST handler in `src/cpp/webd.cpp` does not. It appends `data/pending.dat`.

12. Fixed-width files are the system of record. The ledger is `data/*.dat`. There is no database. The files are ASCII, with an implied decimal, so the C++ programs can share them with GnuCOBOL. A core of this era would more often have used packed decimal and VSAM. The batch programs, copybooks, and 6-digit dates are the mainframe shape that remains.
