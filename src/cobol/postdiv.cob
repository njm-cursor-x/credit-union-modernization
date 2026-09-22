       IDENTIFICATION DIVISION.
       PROGRAM-ID. POSTDIV.
       ENVIRONMENT DIVISION.
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT ACCOUNT-FILE ASSIGN TO "data/accounts.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-ACCT-FS.
           SELECT TXN-FILE ASSIGN TO "data/txns.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-TXN-FS.
           SELECT CONTROL-FILE ASSIGN TO "data/control.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-CTRL-FS.
       DATA DIVISION.
       FILE SECTION.
       FD ACCOUNT-FILE.
       01 ACCOUNT-REC PIC X(33).
       FD TXN-FILE.
       01 TXN-REC PIC X(56).
       FD CONTROL-FILE.
       01 CONTROL-REC PIC X(12).
       WORKING-STORAGE SECTION.
      * 0.15 percent for the month. Not a parameter file.
      * Retirement NAVs in funds.dat are left alone.
       01 WS-DIV-RATE PIC 9V99 VALUE 0.15.
       01 WS-DIV-AMT PIC 9(9)V99 VALUE 0.
       01 WS-ACCT-FS PIC XX.
       01 WS-TXN-FS PIC XX.
       01 WS-CTRL-FS PIC XX.
       01 WS-EOF PIC X VALUE "N".
       01 WS-ACCT-COUNT PIC 9(4) VALUE 0.
       01 WS-NEW-COUNT PIC 9(4) VALUE 0.
       01 WS-I PIC 9(4) VALUE 0.
       01 WS-MAX-SEQ PIC 9(6) VALUE 0.
       01 WS-PAID PIC 9(4) VALUE 0.
       01 WS-DATE-X PIC X(6).
       01 WS-DATE REDEFINES WS-DATE-X PIC 9(6).
       01 WS-TIME-X PIC X(8).
       01 WS-TIME REDEFINES WS-TIME-X PIC 9(8).
       01 WS-STAMP PIC X(12).
       01 ACCT-WORK.
               COPY "account.cpy".
       01 TXN-WORK.
               COPY "txn.cpy".
       01 ACCT-TABLE.
               05 ACCT-HOLD OCCURS 80 TIMES PIC X(33).
       01 NEW-TXN-TABLE.
               05 NEW-TXN OCCURS 80 TIMES PIC X(56).
       PROCEDURE DIVISION.
       MAIN-PARA.
               PERFORM LOAD-ACCOUNTS
               PERFORM SCAN-TXNS
               ACCEPT WS-DATE FROM DATE
               PERFORM VARYING WS-I FROM 1 BY 1
                   UNTIL WS-I > WS-ACCT-COUNT
                   PERFORM PAY-ONE
               END-PERFORM
               PERFORM STORE-ACCOUNTS
               PERFORM STORE-TXNS
               PERFORM STORE-CONTROL
               DISPLAY "POSTDIV DIVIDENDS " WS-PAID
               STOP RUN.
       LOAD-ACCOUNTS.
               OPEN INPUT ACCOUNT-FILE
               IF WS-ACCT-FS NOT = "00"
                   DISPLAY "ACCOUNTS OPEN " WS-ACCT-FS
                   STOP RUN RETURNING 1
               END-IF
               MOVE "N" TO WS-EOF
               PERFORM UNTIL WS-EOF = "Y"
                   READ ACCOUNT-FILE
                       AT END
                           MOVE "Y" TO WS-EOF
                       NOT AT END
                           ADD 1 TO WS-ACCT-COUNT
                           MOVE ACCOUNT-REC TO
                               ACCT-HOLD (WS-ACCT-COUNT)
                   END-READ
               END-PERFORM
               CLOSE ACCOUNT-FILE.
       SCAN-TXNS.
               OPEN INPUT TXN-FILE
               IF WS-TXN-FS NOT = "00"
                   DISPLAY "TXN OPEN " WS-TXN-FS
                   STOP RUN RETURNING 1
               END-IF
               MOVE "N" TO WS-EOF
               PERFORM UNTIL WS-EOF = "Y"
                   READ TXN-FILE
                       AT END
                           MOVE "Y" TO WS-EOF
                       NOT AT END
                           MOVE TXN-REC TO TXN-WORK
                           IF TXN-SEQ > WS-MAX-SEQ
                               MOVE TXN-SEQ TO WS-MAX-SEQ
                           END-IF
                   END-READ
               END-PERFORM
               CLOSE TXN-FILE.
       PAY-ONE.
               MOVE ACCT-HOLD (WS-I) TO ACCT-WORK
               IF ACCT-TYPE = "S" AND ACCT-STATUS = "A"
                   COMPUTE WS-DIV-AMT ROUNDED =
                       ACCT-BAL * WS-DIV-RATE / 100
                   IF WS-DIV-AMT > 0
                       ADD WS-DIV-AMT TO ACCT-BAL
                       MOVE ACCT-WORK TO ACCT-HOLD (WS-I)
                       PERFORM QUEUE-DIV
                   END-IF
               END-IF.
       QUEUE-DIV.
               ADD 1 TO WS-MAX-SEQ
               ADD 1 TO WS-NEW-COUNT
               ADD 1 TO WS-PAID
               MOVE WS-MAX-SEQ TO TXN-SEQ
               MOVE ACCT-NUM TO TXN-ACCT
               MOVE WS-DATE-X TO TXN-DATE
               MOVE "C" TO TXN-DC
               MOVE WS-DIV-AMT TO TXN-AMT
               MOVE "MONTHLY DIVIDEND" TO TXN-DESC
               MOVE TXN-WORK TO NEW-TXN (WS-NEW-COUNT).
       STORE-ACCOUNTS.
               OPEN OUTPUT ACCOUNT-FILE
               PERFORM VARYING WS-I FROM 1 BY 1
                   UNTIL WS-I > WS-ACCT-COUNT
                   MOVE ACCT-HOLD (WS-I) TO ACCOUNT-REC
                   WRITE ACCOUNT-REC
               END-PERFORM
               CLOSE ACCOUNT-FILE.
       STORE-TXNS.
               IF WS-NEW-COUNT > 0
                   OPEN EXTEND TXN-FILE
                   PERFORM VARYING WS-I FROM 1 BY 1
                       UNTIL WS-I > WS-NEW-COUNT
                       MOVE NEW-TXN (WS-I) TO TXN-REC
                       WRITE TXN-REC
                   END-PERFORM
                   CLOSE TXN-FILE
               END-IF.
       STORE-CONTROL.
               ACCEPT WS-DATE FROM DATE
               ACCEPT WS-TIME FROM TIME
               MOVE WS-DATE-X TO WS-STAMP (1:6)
               MOVE WS-TIME-X (1:6) TO WS-STAMP (7:6)
               MOVE WS-STAMP TO CONTROL-REC
               OPEN OUTPUT CONTROL-FILE
               WRITE CONTROL-REC
               CLOSE CONTROL-FILE.
