# Modernization backlog

This is an inventory of what is old at LameX Credit Union. Each item names the file and the behavior a later modernization would replace. Nothing in this list is fixed here.

1. Plaintext PIN and SSN. `data/members.dat` stores the 4-digit PIN and the SSN in the clear. `src/cpp/webd.cpp` signs a member on by comparing that PIN. `src/cobol/stmt.cob` prints the SSN on the statement.

2. Stale balances until batch. The teller and the website append `data/pending.dat`. Balances in `data/accounts.dat` move only when `src/cobol/postpend.cob` runs. Refreshing the site before the batch still shows the previous nightly balance.

3. Teller and batch disagree on overdrafts. `src/cpp/teller.cpp` allows a $50 courtesy overdraft on a share draft withdrawal. `src/cobol/postpend.cob` refuses any debit that would take a balance below zero and writes the line to `data/reject.dat`.

4. No file locking. `src/cpp/teller.cpp`, `src/cpp/webd.cpp`, and the COBOL programs open the fixed-width files with no lock. Two writers can interleave a record.

5. Six-digit dates and magic codes. Dates are `YYMMDD` in `src/cpp/layout.h` and the copybooks under `src/cobol/copy/`. Status is one letter (`A` active, `C` closed, `F` frozen). Account type is one letter (`S` share, `D` draft, `L` loan, `R` retirement). Posted activity is `C` or `D`.

6. Dividend rate hardcoded in COBOL. `src/cobol/postdiv.cob` posts 0.15 percent for the month from a `WORKING-STORAGE` literal. There is no rate file. Retirement NAVs are not part of that job.

7. Retirement prices live in a flat file. `data/funds.dat` holds the fund id, name, NAV, and as-of date. `data/navhist.dat` and `data/valuehist.dat` hold ten years of monthly NAV and market value beside it. Nothing reprices those rows. Someone at the credit union edits the files.

8. No online trading. The holdings page served by `src/cpp/webd.cpp` lists fund, shares, price, and market value, then tells the member to call the branch. The site cannot place a trade.

9. Contributions do not buy shares. `src/cobol/postpend.cob` increases the retirement cash balance when a transfer or deposit posts into a type `R` account. It does not update `data/positions.dat`, so the posted balance and the holdings total can diverge after a contribution.

10. Unsigned session cookie. `src/cpp/webd.cpp` stores the member number in the cookie `LameXMember` with no signature and no expiry. The browser that presents the cookie is that member.

11. Balance check only in the browser for transfers. `web/bank.js` compares the typed transfer amount with the balance shown on the form and can block the submit. The POST handler in `src/cpp/webd.cpp` does not. It appends `data/pending.dat`. Withdrawals, bill payments, and loan payments are checked in the browser and again in the server before that append. The transfer path is the one that still skips the server check.

12. Fixed-width files are the system of record. The ledger is `data/*.dat`. There is no database. The files are ASCII, with an implied decimal, so the C++ programs can share them with GnuCOBOL. A core of this era would more often have used packed decimal and VSAM. The batch programs, copybooks, and 6-digit dates are the mainframe shape that remains.

13. Profile rewrite with no audit. The profile page rewrites `data/members.dat` in place when the mailing address, phone, or email changes. There is no before-image and no journal of who changed what.

14. Plaintext PIN change. The PIN page compares the current PIN in the clear and writes the new 4-digit PIN back into `data/members.dat` in the clear. The write is immediate. It does not wait for the batch.

15. Payee file. `data/payees.dat` is a flat list of billers for a member. Bill pay does not talk to a processor. It appends a `WDL` row to `data/pending.dat` with the payee's description.

16. Full-file history scan. The history page reads all of `data/txns.dat` on every request and keeps the rows in one buffer so it can paint the All-years table. The fund page reads all of `data/navhist.dat` and `data/valuehist.dat` the same way. There is no index and no year is stored apart from the `YYMMDD` on each row.

17. NAV history in a flat file. Monthly NAVs and share values sit in `data/navhist.dat` and `data/valuehist.dat` next to `data/funds.dat`. The latest NAV row is a copy of the price on the fund file. A query for one fund still walks the whole history file.

18. Replace the backend with a stateless service and PostgreSQL. A later modernization stops using `data/*.dat` read by the single-threaded `webd` process and the COBOL batch as the system of record. The target shape is a stateless HTTP service in front of PostgreSQL, so more than one instance can run, posting is transactional, and history and NAV queries are not full-file scans. The retirement display also uses an external quote, described in the next item. That service is not part of this tree.

19. Retirement value on screen comes from stored NAVs. The legacy holdings page multiplies Elena's share counts by the prices in `data/funds.dat`, and the fund history page reads `data/navhist.dat`. A modernization demo shows retirement value from an external real-time market source instead: share count times a live price, not those stored files. The on-screen total may differ from the stored ledger. This is a demo, not a production-grade feed. It does not add trading, compliance, or failover requirements. The target shape stays a stateless HTTP service and PostgreSQL, plus that external quote for the retirement display.
