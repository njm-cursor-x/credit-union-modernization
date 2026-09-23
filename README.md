This repository is LameX Credit Union, and starting it is not a modernization.

You run it on your own machine. Paths are relative to the repository root. There are no environment variables and no database. The site listens on `127.0.0.1:8080`.

## Prerequisites

Linux:

```sh
sudo apt-get install -y build-essential gnucobol libncurses-dev
```

macOS: install the Xcode command-line tools, then:

```sh
brew install gnucobol ncurses
```

## Build

From the repository root:

```sh
make
```

That builds the COBOL batch programs, the teller, and the member-site server into `bin/`.

```sh
sh tests/run.sh
```

The test copies `data/` to a temp directory, drives the teller in command mode, and runs `postpend`.

## Start

An agent should start the site in the background:

```sh
make serve-bg
```

That writes `webd.pid` and `webd.log`. A person at the keyboard can use `make serve` instead and stop it with Ctrl-C.

## Verify

```sh
curl -sf http://127.0.0.1:8080/
```

The response is the HomeBanking sign-on page and includes member `100042`.

## Demo login

Elena Vasquez, member `100042`, PIN `2468`. The same credentials are printed in the Demonstration access box on the sign-on page.

1. Open `http://127.0.0.1:8080/` and sign on as Elena.
2. The accounts page lists Regular Share `10004201`, Share Draft `10004202`, Retirement `10004203`, and Loan `10004204`, plus the last nightly posting time.
3. Open Share Draft. The history page defaults to All years. Use the year dropdown to narrow the table. Scroll the full list.
4. Open Regular Share and the loan and scroll their history. The loan still has a balance.
5. Open Retirement `10004203`. The holdings table lists VFIAX, VBTLX, VTIAX, and VMFXX with shares, the stored price, market value, and a total. Each fund name links to a month-by-month history table.
6. Invest: in the Invest table, choose Regular Share or Share Draft, choose one fund, enter dollars, and schedule the invest. Open Pending. The instruction is listed. The balance and the shares have not moved.
7. Exchange: in the Exchange table, choose a fund she holds, choose another fund, enter dollars, and schedule the exchange. Pending lists that instruction too.
8. Submit a transfer from Share Draft to Regular Share. The page says the transfer is scheduled. The balance does not move yet.
9. Schedule a deposit, a withdrawal, a bill payment (electric, phone, insurance, or credit card), and a loan payment. Open Pending. The balances still have not moved.
10. Open Profile and change the mailing address, phone, or email. Name and SSN stay as they are. Open PIN to replace the 4-digit PIN. Those two writes hit the member file immediately.
11. Optionally run `make batch`, reload, and the pending items are in history. Holdings show the new share counts. Transaction history shows `INVEST` or `EXCH` with the fund ids. An exchange does not take cash out of the retirement account. A loan payment reduces the amount owed. A payment larger than the loan balance is rejected by the batch. An invest or exchange the stored balance cannot cover is written to `data/reject.dat` and does not change positions.

In another terminal, from the repository root:

```sh
make teller
```

Sign on as teller `0001` / `WESTON01`, find member `100042`, and post a deposit. Reload the site. The balance is unchanged until:

```sh
make batch
```

`make batch` runs `postpend` and then `postdiv`. A share-draft withdrawal that goes no more than $50 below zero is accepted at the teller (courtesy pay) and rejected by `postpend`, which writes `data/reject.dat`.

## Stop

```sh
make stop
```

## Data

Member, account, transaction, pending, teller, control, fund, position, NAV history, value history, and payee files live in `data/`. The C++ programs and the COBOL programs both read those paths from the repository root. Do not point them at a database.

`src/cpp/seedhist.cpp` is the program that wrote Elena's ten-year history and the retirement files. `make` does not run it. The generated files are already in `data/`.

What is old, and would be replaced in a later modernization, is listed in [docs/modernization-backlog.md](docs/modernization-backlog.md).
