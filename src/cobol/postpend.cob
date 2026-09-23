       IDENTIFICATION DIVISION.
       PROGRAM-ID. POSTPEND.
       ENVIRONMENT DIVISION.
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT ACCOUNT-FILE ASSIGN TO "data/accounts.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-ACCT-FS.
           SELECT TXN-FILE ASSIGN TO "data/txns.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-TXN-FS.
           SELECT PENDING-FILE ASSIGN TO "data/pending.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-PEND-FS.
           SELECT REJECT-FILE ASSIGN TO "data/reject.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-REJ-FS.
           SELECT CONTROL-FILE ASSIGN TO "data/control.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-CTRL-FS.
           SELECT FUND-FILE ASSIGN TO "data/funds.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-FUND-FS.
           SELECT POSITION-FILE ASSIGN TO "data/positions.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-POS-FS.
       DATA DIVISION.
       FILE SECTION.
       FD ACCOUNT-FILE.
       01 ACCOUNT-REC PIC X(33).
       FD TXN-FILE.
       01 TXN-REC PIC X(56).
       FD PENDING-FILE.
       01 PENDING-REC PIC X(65).
       FD REJECT-FILE.
       01 REJECT-REC PIC X(105).
       FD CONTROL-FILE.
       01 CONTROL-REC PIC X(12).
       FD FUND-FILE.
       01 FUND-REC PIC X(70).
       FD POSITION-FILE.
       01 POSITION-REC PIC X(24).
       WORKING-STORAGE SECTION.
       01 WS-ACCT-FS PIC XX.
       01 WS-TXN-FS PIC XX.
       01 WS-PEND-FS PIC XX.
       01 WS-REJ-FS PIC XX.
       01 WS-CTRL-FS PIC XX.
       01 WS-FUND-FS PIC XX.
       01 WS-POS-FS PIC XX.
       01 WS-EOF PIC X VALUE "N".
       01 WS-FOUND PIC X VALUE "N".
       01 WS-FROM-OK PIC X VALUE "N".
       01 WS-TO-OK PIC X VALUE "N".
       01 WS-ACCT-COUNT PIC 9(4) VALUE 0.
       01 WS-PEND-COUNT PIC 9(4) VALUE 0.
       01 WS-NEW-COUNT PIC 9(4) VALUE 0.
       01 WS-REJ-COUNT PIC 9(4) VALUE 0.
       01 WS-I PIC 9(4) VALUE 0.
       01 WS-P PIC 9(4) VALUE 0.
       01 WS-IDX PIC 9(4) VALUE 0.
       01 WS-FROM-IDX PIC 9(4) VALUE 0.
       01 WS-TO-IDX PIC 9(4) VALUE 0.
       01 WS-POS-IDX PIC 9(4) VALUE 0.
       01 WS-DST-IDX PIC 9(4) VALUE 0.
       01 WS-FUND-COUNT PIC 9(4) VALUE 0.
       01 WS-POS-COUNT PIC 9(4) VALUE 0.
       01 WS-J PIC 9(4) VALUE 0.
       01 WS-MAX-SEQ PIC 9(6) VALUE 0.
       01 WS-KEY PIC X(8).
       01 WS-AMT PIC 9(9)V99 VALUE 0.
       01 WS-POS-DIRTY PIC X VALUE "N".
       01 WS-CALC-OK PIC X VALUE "N".
       01 WS-POS-NEW PIC X VALUE "N".
       01 WS-MBR-A PIC X(6).
       01 WS-MBR-B PIC X(6).
       01 WS-FUND-ID PIC X(5).
       01 WS-FUND-SRC PIC X(5).
       01 WS-FUND-DST PIC X(5).
       01 WS-POS-KEY PIC X(8).
       01 WS-FUND-KEY PIC X(5).
       01 WS-NAV-DIGITS PIC 9(11) VALUE 0.
       01 WS-NAV-SCALED REDEFINES WS-NAV-DIGITS PIC 9(9)V99.
       01 WS-AMT-DIGITS PIC 9(11) VALUE 0.
       01 WS-AMT-SCALED REDEFINES WS-AMT-DIGITS PIC 9(9)V99.
       01 WS-SHR-DIGITS PIC 9(11) VALUE 0.
       01 WS-SHR-SCALED REDEFINES WS-SHR-DIGITS PIC 9(7)V9(4).
       01 WS-SELL-DIGITS PIC 9(11) VALUE 0.
       01 WS-BUY-DIGITS PIC 9(11) VALUE 0.
       01 WS-PROD PIC 9(18) VALUE 0.
       01 WS-DC PIC X.
       01 WS-TXN-ACCT-ARG PIC X(8).
       01 WS-TXN-DATE-ARG PIC X(6).
       01 WS-TXN-DESC-ARG PIC X(24).
       01 WS-MSG PIC X(40).
       01 WS-DATE-X PIC X(6).
       01 WS-DATE REDEFINES WS-DATE-X PIC 9(6).
       01 WS-TIME-X PIC X(8).
       01 WS-TIME REDEFINES WS-TIME-X PIC 9(8).
       01 WS-STAMP PIC X(12).
       01 ACCT-WORK.
               COPY "account.cpy".
       01 PEND-WORK.
               COPY "pending.cpy".
       01 TXN-WORK.
               COPY "txn.cpy".
       01 REJECT-WORK.
               COPY "reject.cpy".
       01 FUND-WORK.
               COPY "fund.cpy".
       01 POS-WORK.
               COPY "position.cpy".
       01 ACCT-TABLE.
               05 ACCT-HOLD OCCURS 80 TIMES PIC X(33).
       01 PEND-TABLE.
               05 PEND-HOLD OCCURS 200 TIMES PIC X(65).
       01 NEW-TXN-TABLE.
               05 NEW-TXN OCCURS 400 TIMES PIC X(56).
       01 REJ-TABLE.
               05 REJ-HOLD OCCURS 200 TIMES PIC X(105).
       01 FUND-TABLE.
               05 FUND-HOLD OCCURS 16 TIMES PIC X(70).
       01 POS-TABLE.
               05 POS-HOLD OCCURS 80 TIMES PIC X(24).
       PROCEDURE DIVISION.
       MAIN-PARA.
               PERFORM LOAD-ACCOUNTS
               PERFORM LOAD-FUNDS
               PERFORM LOAD-POSITIONS
               PERFORM SCAN-TXNS
               PERFORM LOAD-PENDING
               PERFORM VARYING WS-P FROM 1 BY 1
                   UNTIL WS-P > WS-PEND-COUNT
                   PERFORM APPLY-PENDING
               END-PERFORM
               PERFORM STORE-ACCOUNTS
               PERFORM STORE-POSITIONS
               PERFORM STORE-TXNS
               PERFORM EMPTY-PENDING
               PERFORM STORE-REJECTS
               PERFORM STORE-CONTROL
               DISPLAY "POSTPEND POSTED " WS-NEW-COUNT
                   " REJECTED " WS-REJ-COUNT
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
                           IF WS-ACCT-COUNT = 80
                               DISPLAY "ACCOUNT TABLE FULL"
                               STOP RUN RETURNING 1
                           END-IF
                           ADD 1 TO WS-ACCT-COUNT
                           MOVE ACCOUNT-REC TO
                               ACCT-HOLD (WS-ACCT-COUNT)
                   END-READ
               END-PERFORM
               CLOSE ACCOUNT-FILE.
       LOAD-FUNDS.
               OPEN INPUT FUND-FILE
               IF WS-FUND-FS NOT = "00"
                   DISPLAY "FUNDS OPEN " WS-FUND-FS
                   STOP RUN RETURNING 1
               END-IF
               MOVE "N" TO WS-EOF
               PERFORM UNTIL WS-EOF = "Y"
                   READ FUND-FILE
                       AT END
                           MOVE "Y" TO WS-EOF
                       NOT AT END
                           IF WS-FUND-COUNT = 16
                               DISPLAY "FUND TABLE FULL"
                               STOP RUN RETURNING 1
                           END-IF
                           ADD 1 TO WS-FUND-COUNT
                           MOVE FUND-REC TO FUND-HOLD (WS-FUND-COUNT)
                   END-READ
               END-PERFORM
               CLOSE FUND-FILE.
       LOAD-POSITIONS.
               OPEN INPUT POSITION-FILE
               IF WS-POS-FS NOT = "00"
                   DISPLAY "POSITIONS OPEN " WS-POS-FS
                   STOP RUN RETURNING 1
               END-IF
               MOVE "N" TO WS-EOF
               PERFORM UNTIL WS-EOF = "Y"
                   READ POSITION-FILE
                       AT END
                           MOVE "Y" TO WS-EOF
                       NOT AT END
                           IF WS-POS-COUNT = 80
                               DISPLAY "POSITION TABLE FULL"
                               STOP RUN RETURNING 1
                           END-IF
                           ADD 1 TO WS-POS-COUNT
                           MOVE POSITION-REC TO
                               POS-HOLD (WS-POS-COUNT)
                   END-READ
               END-PERFORM
               CLOSE POSITION-FILE.
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
       LOAD-PENDING.
               OPEN INPUT PENDING-FILE
               IF WS-PEND-FS = "35"
                   MOVE 0 TO WS-PEND-COUNT
               ELSE
                   IF WS-PEND-FS NOT = "00"
                       DISPLAY "PENDING OPEN " WS-PEND-FS
                       STOP RUN RETURNING 1
                   END-IF
                   MOVE "N" TO WS-EOF
                   PERFORM UNTIL WS-EOF = "Y"
                       READ PENDING-FILE
                           AT END
                               MOVE "Y" TO WS-EOF
                           NOT AT END
                               IF WS-PEND-COUNT = 200
                                   DISPLAY "PENDING TABLE FULL"
                                   STOP RUN RETURNING 1
                               END-IF
                               ADD 1 TO WS-PEND-COUNT
                               MOVE PENDING-REC TO
                                   PEND-HOLD (WS-PEND-COUNT)
                       END-READ
                   END-PERFORM
                   CLOSE PENDING-FILE
               END-IF.
       APPLY-PENDING.
               MOVE PEND-HOLD (WS-P) TO PEND-WORK
               MOVE PEND-AMT TO WS-AMT
               MOVE PEND-DATE TO WS-TXN-DATE-ARG
               IF PEND-AMT = 0
                   MOVE "AMOUNT IS ZERO" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   EVALUATE PEND-CODE
                       WHEN "DEP"
                           PERFORM APPLY-DEP
                       WHEN "WDL"
                           PERFORM APPLY-WDL
                       WHEN "XFR"
                           PERFORM APPLY-XFR
                       WHEN "INV"
                           PERFORM APPLY-INV
                       WHEN "XCH"
                           PERFORM APPLY-XCH
                       WHEN OTHER
                           MOVE "UNKNOWN TRAN CODE" TO WS-MSG
                           PERFORM QUEUE-REJECT
                   END-EVALUATE
               END-IF.
       APPLY-DEP.
               MOVE PEND-FROM TO WS-KEY
               PERFORM LOCATE-ACCT
               IF WS-FOUND NOT = "Y"
                   MOVE "ACCOUNT NOT FOUND" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   IF ACCT-STATUS NOT = "A"
                       MOVE "ACCOUNT NOT ACTIVE" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
      * DEP and XFR still only move cash. INV and XCH
      * are the codes that change positions.dat.
                       ADD PEND-AMT TO ACCT-BAL
                       MOVE ACCT-WORK TO ACCT-HOLD (WS-IDX)
                       MOVE "C" TO WS-DC
                       MOVE PEND-FROM TO WS-TXN-ACCT-ARG
                       IF PEND-DESC = SPACES
                           MOVE "DEPOSIT" TO WS-TXN-DESC-ARG
                       ELSE
                           MOVE PEND-DESC TO WS-TXN-DESC-ARG
                       END-IF
                       PERFORM QUEUE-TXN
                   END-IF
               END-IF.
       APPLY-WDL.
               MOVE PEND-FROM TO WS-KEY
               PERFORM LOCATE-ACCT
               IF WS-FOUND NOT = "Y"
                   MOVE "ACCOUNT NOT FOUND" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   IF ACCT-STATUS NOT = "A"
                       MOVE "ACCOUNT NOT ACTIVE" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       IF ACCT-BAL < PEND-AMT
      * Share and draft may not go below zero. Courtesy
      * pay accepted by the teller is rejected here.
      * A type L loan payment is this same WDL. The test
      * rejects a payment larger than the amount owed.
                           MOVE "NSF REJECTED BY BATCH" TO WS-MSG
                           PERFORM QUEUE-REJECT
                       ELSE
                           SUBTRACT PEND-AMT FROM ACCT-BAL
                           MOVE ACCT-WORK TO ACCT-HOLD (WS-IDX)
                           MOVE "D" TO WS-DC
                           MOVE PEND-FROM TO WS-TXN-ACCT-ARG
                           IF PEND-DESC = SPACES
                               MOVE "WITHDRAWAL" TO WS-TXN-DESC-ARG
                           ELSE
                               MOVE PEND-DESC TO WS-TXN-DESC-ARG
                           END-IF
                           PERFORM QUEUE-TXN
                       END-IF
                   END-IF
               END-IF.
       APPLY-XFR.
               IF PEND-FROM = PEND-TO
                   MOVE "TRANSFER ACCOUNTS MATCH" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   MOVE PEND-FROM TO WS-KEY
                   PERFORM LOCATE-ACCT
                   MOVE WS-FOUND TO WS-FROM-OK
                   MOVE WS-IDX TO WS-FROM-IDX
                   MOVE PEND-TO TO WS-KEY
                   PERFORM LOCATE-ACCT
                   MOVE WS-FOUND TO WS-TO-OK
                   MOVE WS-IDX TO WS-TO-IDX
                   IF WS-FROM-OK NOT = "Y" OR WS-TO-OK NOT = "Y"
                       MOVE "ACCOUNT NOT FOUND" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       PERFORM APPLY-XFR-BODY
                   END-IF
               END-IF.
       APPLY-XFR-BODY.
               MOVE ACCT-HOLD (WS-FROM-IDX) TO ACCT-WORK
               IF ACCT-STATUS NOT = "A"
                   MOVE "ACCOUNT NOT ACTIVE" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   MOVE ACCT-HOLD (WS-TO-IDX) TO ACCT-WORK
                   IF ACCT-STATUS NOT = "A"
                       MOVE "ACCOUNT NOT ACTIVE" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       MOVE ACCT-HOLD (WS-FROM-IDX) TO ACCT-WORK
                       IF ACCT-BAL < PEND-AMT
                           MOVE "NSF REJECTED BY BATCH" TO WS-MSG
                           PERFORM QUEUE-REJECT
                       ELSE
                           SUBTRACT PEND-AMT FROM ACCT-BAL
                           MOVE ACCT-WORK TO ACCT-HOLD (WS-FROM-IDX)
                           MOVE ACCT-HOLD (WS-TO-IDX) TO ACCT-WORK
                           ADD PEND-AMT TO ACCT-BAL
                           MOVE ACCT-WORK TO ACCT-HOLD (WS-TO-IDX)
                           MOVE "D" TO WS-DC
                           MOVE PEND-FROM TO WS-TXN-ACCT-ARG
                           PERFORM SET-XFR-DESC
                           PERFORM QUEUE-TXN
                           MOVE "C" TO WS-DC
                           MOVE PEND-TO TO WS-TXN-ACCT-ARG
                           PERFORM QUEUE-TXN
                       END-IF
                   END-IF
               END-IF.
       SET-XFR-DESC.
               IF PEND-DESC = SPACES
                   MOVE "TRANSFER" TO WS-TXN-DESC-ARG
               ELSE
                   MOVE PEND-DESC TO WS-TXN-DESC-ARG
               END-IF.
      * The 65-byte pending row has no fund columns. INV stores the
      * 5-character fund id in PEND-DESC. XCH stores the source fund
      * in columns 1-5 and the destination fund in columns 6-10.
       APPLY-INV.
               MOVE PEND-DESC (1:5) TO WS-FUND-ID
               MOVE PEND-FROM TO WS-KEY
               PERFORM LOCATE-ACCT
               IF WS-FOUND NOT = "Y"
                   MOVE "ACCOUNT NOT FOUND" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   MOVE WS-IDX TO WS-FROM-IDX
                   MOVE ACCT-MBR TO WS-MBR-A
                   IF ACCT-STATUS NOT = "A"
                       MOVE "ACCOUNT NOT ACTIVE" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       IF ACCT-TYPE NOT = "S" AND ACCT-TYPE NOT = "D"
                           MOVE "SOURCE NOT SHARE OR DRAFT" TO WS-MSG
                           PERFORM QUEUE-REJECT
                       ELSE
                           PERFORM APPLY-INV-DEST
                       END-IF
                   END-IF
               END-IF.
       APPLY-INV-DEST.
               MOVE PEND-TO TO WS-KEY
               PERFORM LOCATE-ACCT
               IF WS-FOUND NOT = "Y"
                   MOVE "ACCOUNT NOT FOUND" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   MOVE WS-IDX TO WS-TO-IDX
                   MOVE ACCT-MBR TO WS-MBR-B
                   IF ACCT-STATUS NOT = "A"
                       MOVE "ACCOUNT NOT ACTIVE" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       IF ACCT-TYPE NOT = "R"
                           MOVE "NOT A RETIREMENT ACCT" TO WS-MSG
                           PERFORM QUEUE-REJECT
                       ELSE
                           IF WS-MBR-A NOT = WS-MBR-B
                               MOVE "MEMBERS DIFFER" TO WS-MSG
                               PERFORM QUEUE-REJECT
                           ELSE
                               PERFORM APPLY-INV-FUND
                           END-IF
                       END-IF
                   END-IF
               END-IF.
       APPLY-INV-FUND.
               MOVE WS-FUND-ID TO WS-FUND-KEY
               PERFORM LOCATE-FUND
               IF WS-FOUND NOT = "Y"
                   MOVE "FUND NOT FOUND" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   MOVE FUND-NAV TO WS-NAV-SCALED
                   MOVE WS-AMT TO WS-AMT-SCALED
                   PERFORM CALC-SHARES
                   IF WS-CALC-OK NOT = "Y"
                       PERFORM QUEUE-REJECT
                   ELSE
                       MOVE ACCT-HOLD (WS-FROM-IDX) TO ACCT-WORK
                       IF ACCT-BAL < WS-AMT
                           MOVE "NSF REJECTED BY BATCH" TO WS-MSG
                           PERFORM QUEUE-REJECT
                       ELSE
                           PERFORM APPLY-INV-BODY
                       END-IF
                   END-IF
               END-IF.
       APPLY-INV-BODY.
               MOVE PEND-TO TO WS-POS-KEY
               MOVE WS-FUND-ID TO WS-FUND-KEY
               PERFORM LOCATE-POS
               IF WS-FOUND = "Y"
                   MOVE WS-IDX TO WS-POS-IDX
                   MOVE "N" TO WS-POS-NEW
                   MOVE POS-SHARES TO WS-SHR-SCALED
                   IF WS-SHR-DIGITS > 99999999999 - WS-BUY-DIGITS
                       MOVE "SHARE COUNT OVERFLOW" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       PERFORM APPLY-INV-POST
                   END-IF
               ELSE
                   IF WS-POS-COUNT = 80
                       MOVE "POSITION TABLE FULL" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       MOVE "Y" TO WS-POS-NEW
                       PERFORM APPLY-INV-POST
                   END-IF
               END-IF.
       APPLY-INV-POST.
               MOVE ACCT-HOLD (WS-FROM-IDX) TO ACCT-WORK
               SUBTRACT WS-AMT FROM ACCT-BAL
               MOVE ACCT-WORK TO ACCT-HOLD (WS-FROM-IDX)
               MOVE ACCT-HOLD (WS-TO-IDX) TO ACCT-WORK
               ADD WS-AMT TO ACCT-BAL
               MOVE ACCT-WORK TO ACCT-HOLD (WS-TO-IDX)
               IF WS-POS-NEW = "Y"
                   ADD 1 TO WS-POS-COUNT
                   MOVE WS-POS-COUNT TO WS-POS-IDX
                   MOVE SPACES TO POS-WORK
                   MOVE WS-POS-KEY TO POS-ACCT
                   MOVE WS-FUND-KEY TO POS-FUND
                   MOVE 0 TO POS-SHARES
               ELSE
                   MOVE POS-HOLD (WS-POS-IDX) TO POS-WORK
               END-IF
               MOVE POS-SHARES TO WS-SHR-SCALED
               ADD WS-BUY-DIGITS TO WS-SHR-DIGITS
               MOVE WS-SHR-SCALED TO POS-SHARES
               MOVE POS-WORK TO POS-HOLD (WS-POS-IDX)
               MOVE "Y" TO WS-POS-DIRTY
               MOVE SPACES TO WS-TXN-DESC-ARG
               STRING "INVEST " DELIMITED BY SIZE
                   WS-FUND-ID DELIMITED BY SIZE
                   INTO WS-TXN-DESC-ARG
               END-STRING
               MOVE "D" TO WS-DC
               MOVE PEND-FROM TO WS-TXN-ACCT-ARG
               PERFORM QUEUE-TXN
               MOVE "C" TO WS-DC
               MOVE PEND-TO TO WS-TXN-ACCT-ARG
               PERFORM QUEUE-TXN.
       APPLY-XCH.
               MOVE PEND-DESC (1:5) TO WS-FUND-SRC
               MOVE PEND-DESC (6:5) TO WS-FUND-DST
               IF WS-FUND-SRC = WS-FUND-DST
                   OR WS-FUND-SRC = SPACES
                   OR WS-FUND-DST = SPACES
                   MOVE "FUNDS MATCH" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   MOVE PEND-FROM TO WS-KEY
                   PERFORM LOCATE-ACCT
                   IF WS-FOUND NOT = "Y"
                       MOVE "ACCOUNT NOT FOUND" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       MOVE WS-IDX TO WS-FROM-IDX
                       IF ACCT-STATUS NOT = "A"
                           MOVE "ACCOUNT NOT ACTIVE" TO WS-MSG
                           PERFORM QUEUE-REJECT
                       ELSE
                           IF ACCT-TYPE NOT = "R"
                               MOVE "NOT A RETIREMENT ACCT" TO WS-MSG
                               PERFORM QUEUE-REJECT
                           ELSE
                               PERFORM APPLY-XCH-FUNDS
                           END-IF
                       END-IF
                   END-IF
               END-IF.
       APPLY-XCH-FUNDS.
               MOVE WS-FUND-SRC TO WS-FUND-KEY
               PERFORM LOCATE-FUND
               IF WS-FOUND NOT = "Y"
                   MOVE "FUND NOT FOUND" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   MOVE FUND-NAV TO WS-NAV-SCALED
                   MOVE WS-AMT TO WS-AMT-SCALED
                   PERFORM CALC-SHARES
                   IF WS-CALC-OK NOT = "Y"
                       PERFORM QUEUE-REJECT
                   ELSE
                       MOVE WS-BUY-DIGITS TO WS-SELL-DIGITS
                       MOVE WS-FUND-DST TO WS-FUND-KEY
                       PERFORM LOCATE-FUND
                       IF WS-FOUND NOT = "Y"
                           MOVE "FUND NOT FOUND" TO WS-MSG
                           PERFORM QUEUE-REJECT
                       ELSE
                           MOVE FUND-NAV TO WS-NAV-SCALED
                           PERFORM CALC-SHARES
                           IF WS-CALC-OK NOT = "Y"
                               PERFORM QUEUE-REJECT
                           ELSE
                               PERFORM APPLY-XCH-POS
                           END-IF
                       END-IF
                   END-IF
               END-IF.
       APPLY-XCH-POS.
               MOVE PEND-FROM TO WS-POS-KEY
               MOVE WS-FUND-SRC TO WS-FUND-KEY
               PERFORM LOCATE-POS
               IF WS-FOUND NOT = "Y"
                   MOVE "INSUFFICIENT SHARES" TO WS-MSG
                   PERFORM QUEUE-REJECT
               ELSE
                   MOVE WS-IDX TO WS-POS-IDX
                   MOVE POS-SHARES TO WS-SHR-SCALED
                   IF WS-SHR-DIGITS < WS-SELL-DIGITS
                       MOVE "INSUFFICIENT SHARES" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       PERFORM APPLY-XCH-DEST
                   END-IF
               END-IF.
       APPLY-XCH-DEST.
               MOVE WS-FUND-DST TO WS-FUND-KEY
               PERFORM LOCATE-POS
               IF WS-FOUND = "Y"
                   MOVE WS-IDX TO WS-DST-IDX
                   MOVE "N" TO WS-POS-NEW
                   MOVE POS-SHARES TO WS-SHR-SCALED
                   IF WS-SHR-DIGITS > 99999999999 - WS-BUY-DIGITS
                       MOVE "SHARE COUNT OVERFLOW" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       PERFORM APPLY-XCH-POST
                   END-IF
               ELSE
                   IF WS-POS-COUNT = 80
                       MOVE "POSITION TABLE FULL" TO WS-MSG
                       PERFORM QUEUE-REJECT
                   ELSE
                       MOVE "Y" TO WS-POS-NEW
                       PERFORM APPLY-XCH-POST
                   END-IF
               END-IF.
       APPLY-XCH-POST.
               MOVE POS-HOLD (WS-POS-IDX) TO POS-WORK
               MOVE POS-SHARES TO WS-SHR-SCALED
               SUBTRACT WS-SELL-DIGITS FROM WS-SHR-DIGITS
               MOVE WS-SHR-SCALED TO POS-SHARES
               MOVE POS-WORK TO POS-HOLD (WS-POS-IDX)
               IF WS-POS-NEW = "Y"
                   ADD 1 TO WS-POS-COUNT
                   MOVE WS-POS-COUNT TO WS-DST-IDX
                   MOVE SPACES TO POS-WORK
                   MOVE PEND-FROM TO POS-ACCT
                   MOVE WS-FUND-DST TO POS-FUND
                   MOVE 0 TO POS-SHARES
               ELSE
                   MOVE POS-HOLD (WS-DST-IDX) TO POS-WORK
               END-IF
               MOVE POS-SHARES TO WS-SHR-SCALED
               ADD WS-BUY-DIGITS TO WS-SHR-DIGITS
               MOVE WS-SHR-SCALED TO POS-SHARES
               MOVE POS-WORK TO POS-HOLD (WS-DST-IDX)
               MOVE "Y" TO WS-POS-DIRTY
               MOVE SPACES TO WS-TXN-DESC-ARG
               STRING "EXCH " DELIMITED BY SIZE
                   WS-FUND-SRC DELIMITED BY SIZE
                   " TO " DELIMITED BY SIZE
                   WS-FUND-DST DELIMITED BY SIZE
                   INTO WS-TXN-DESC-ARG
               END-STRING
               MOVE "D" TO WS-DC
               MOVE PEND-FROM TO WS-TXN-ACCT-ARG
               PERFORM QUEUE-TXN
               MOVE "C" TO WS-DC
               PERFORM QUEUE-TXN.
       CALC-SHARES.
               MOVE "Y" TO WS-CALC-OK
               MOVE 0 TO WS-BUY-DIGITS
               IF WS-NAV-DIGITS = 0
                   MOVE "N" TO WS-CALC-OK
                   MOVE "NAV IS ZERO" TO WS-MSG
               ELSE
                   COMPUTE WS-PROD = WS-AMT-DIGITS * 10000
                       ON SIZE ERROR
                           MOVE "N" TO WS-CALC-OK
                           MOVE "SHARE COUNT OVERFLOW" TO WS-MSG
                   END-COMPUTE
                   IF WS-CALC-OK = "Y"
                       DIVIDE WS-NAV-DIGITS INTO WS-PROD
                           GIVING WS-BUY-DIGITS
                           ON SIZE ERROR
                               MOVE "N" TO WS-CALC-OK
                               MOVE "SHARE COUNT OVERFLOW" TO WS-MSG
                       END-DIVIDE
                   END-IF
                   IF WS-CALC-OK = "Y" AND WS-BUY-DIGITS = 0
                       MOVE "N" TO WS-CALC-OK
                       MOVE "AMOUNT TOO SMALL" TO WS-MSG
                   END-IF
               END-IF.
       LOCATE-FUND.
               MOVE "N" TO WS-FOUND
               MOVE 0 TO WS-IDX
               PERFORM VARYING WS-J FROM 1 BY 1
                   UNTIL WS-J > WS-FUND-COUNT
                   OR WS-FOUND = "Y"
                   MOVE FUND-HOLD (WS-J) TO FUND-WORK
                   IF FUND-ID = WS-FUND-KEY
                       MOVE "Y" TO WS-FOUND
                       MOVE WS-J TO WS-IDX
                   END-IF
               END-PERFORM.
       LOCATE-POS.
               MOVE "N" TO WS-FOUND
               MOVE 0 TO WS-IDX
               PERFORM VARYING WS-J FROM 1 BY 1
                   UNTIL WS-J > WS-POS-COUNT
                   OR WS-FOUND = "Y"
                   MOVE POS-HOLD (WS-J) TO POS-WORK
                   IF POS-ACCT = WS-POS-KEY
                       AND POS-FUND = WS-FUND-KEY
                       MOVE "Y" TO WS-FOUND
                       MOVE WS-J TO WS-IDX
                   END-IF
               END-PERFORM.
       LOCATE-ACCT.
               MOVE "N" TO WS-FOUND
               MOVE 0 TO WS-IDX
               PERFORM VARYING WS-I FROM 1 BY 1
                   UNTIL WS-I > WS-ACCT-COUNT
                   OR WS-FOUND = "Y"
                   MOVE ACCT-HOLD (WS-I) TO ACCT-WORK
                   IF ACCT-NUM = WS-KEY
                       MOVE "Y" TO WS-FOUND
                       MOVE WS-I TO WS-IDX
                   END-IF
               END-PERFORM.
       QUEUE-TXN.
               IF WS-NEW-COUNT = 400
                   DISPLAY "TXN TABLE FULL"
                   STOP RUN RETURNING 1
               END-IF
               ADD 1 TO WS-MAX-SEQ
               ADD 1 TO WS-NEW-COUNT
               MOVE WS-MAX-SEQ TO TXN-SEQ
               MOVE WS-TXN-ACCT-ARG TO TXN-ACCT
               MOVE WS-TXN-DATE-ARG TO TXN-DATE
               MOVE WS-DC TO TXN-DC
               MOVE WS-AMT TO TXN-AMT
               MOVE WS-TXN-DESC-ARG TO TXN-DESC
               MOVE TXN-WORK TO NEW-TXN (WS-NEW-COUNT).
       QUEUE-REJECT.
               IF WS-REJ-COUNT = 200
                   DISPLAY "REJECT TABLE FULL"
                   STOP RUN RETURNING 1
               END-IF
               ADD 1 TO WS-REJ-COUNT
               MOVE PEND-WORK TO REJ-PEND
               MOVE WS-MSG TO REJ-REASON
               MOVE REJECT-WORK TO REJ-HOLD (WS-REJ-COUNT).
       STORE-ACCOUNTS.
               OPEN OUTPUT ACCOUNT-FILE
               IF WS-ACCT-FS NOT = "00"
                   DISPLAY "ACCOUNTS OUT " WS-ACCT-FS
                   STOP RUN RETURNING 1
               END-IF
               PERFORM VARYING WS-I FROM 1 BY 1
                   UNTIL WS-I > WS-ACCT-COUNT
                   MOVE ACCT-HOLD (WS-I) TO ACCOUNT-REC
                   WRITE ACCOUNT-REC
               END-PERFORM
               CLOSE ACCOUNT-FILE.
       STORE-POSITIONS.
               IF WS-POS-DIRTY = "Y"
                   OPEN OUTPUT POSITION-FILE
                   IF WS-POS-FS NOT = "00"
                       DISPLAY "POSITIONS OUT " WS-POS-FS
                       STOP RUN RETURNING 1
                   END-IF
                   PERFORM VARYING WS-I FROM 1 BY 1
                       UNTIL WS-I > WS-POS-COUNT
                       MOVE POS-HOLD (WS-I) TO POSITION-REC
                       WRITE POSITION-REC
                   END-PERFORM
                   CLOSE POSITION-FILE
               END-IF.
       STORE-TXNS.
               IF WS-NEW-COUNT > 0
                   OPEN EXTEND TXN-FILE
                   IF WS-TXN-FS NOT = "00"
                       DISPLAY "TXN EXTEND " WS-TXN-FS
                       STOP RUN RETURNING 1
                   END-IF
                   PERFORM VARYING WS-I FROM 1 BY 1
                       UNTIL WS-I > WS-NEW-COUNT
                       MOVE NEW-TXN (WS-I) TO TXN-REC
                       WRITE TXN-REC
                   END-PERFORM
                   CLOSE TXN-FILE
               END-IF.
       EMPTY-PENDING.
               OPEN OUTPUT PENDING-FILE
               IF WS-PEND-FS NOT = "00"
                   DISPLAY "PENDING CLEAR " WS-PEND-FS
                   STOP RUN RETURNING 1
               END-IF
               CLOSE PENDING-FILE.
       STORE-REJECTS.
               OPEN OUTPUT REJECT-FILE
               IF WS-REJ-FS NOT = "00"
                   DISPLAY "REJECT OPEN " WS-REJ-FS
                   STOP RUN RETURNING 1
               END-IF
               PERFORM VARYING WS-I FROM 1 BY 1
                   UNTIL WS-I > WS-REJ-COUNT
                   MOVE REJ-HOLD (WS-I) TO REJECT-REC
                   WRITE REJECT-REC
               END-PERFORM
               CLOSE REJECT-FILE.
       STORE-CONTROL.
               ACCEPT WS-DATE FROM DATE
               ACCEPT WS-TIME FROM TIME
               MOVE WS-DATE-X TO WS-STAMP (1:6)
               MOVE WS-TIME-X (1:6) TO WS-STAMP (7:6)
               MOVE WS-STAMP TO CONTROL-REC
               OPEN OUTPUT CONTROL-FILE
               IF WS-CTRL-FS NOT = "00"
                   DISPLAY "CONTROL OPEN " WS-CTRL-FS
                   STOP RUN RETURNING 1
               END-IF
               WRITE CONTROL-REC
               CLOSE CONTROL-FILE.
