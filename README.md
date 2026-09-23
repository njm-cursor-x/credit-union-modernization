This repository is the legacy credit union, and starting it is not a modernization.

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
2. The accounts page lists Regular Share `10004201`, Share Draft `10004202`, and Retirement `10004203`, plus the last nightly posting time.
3. Open Regular Share and Share Draft and scroll the transaction list.
4. Open Retirement and read the holdings table: fund, shares, price, market value, and total. The page says to call the branch. It does not place trades.
5. Submit a transfer from Share Draft to Regular Share. The page says the transfer is scheduled. The balance does not move yet.
6. Optionally run `make batch`, reload, and the transfer is in history.

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

Member, account, transaction, pending, teller, control, fund, and position files live in `data/`. The C++ programs and the COBOL programs both read those paths from the repository root. Do not point them at a database.

What is old, and would be replaced in a later modernization, is listed in [docs/modernization-backlog.md](docs/modernization-backlog.md).
