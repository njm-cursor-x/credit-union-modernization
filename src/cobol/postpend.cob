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
       WORKING-STORAGE SECTION.
       01 WS-ACCT-FS PIC XX.
       01 WS-TXN-FS PIC XX.
       01 WS-PEND-FS PIC XX.
       01 WS-REJ-FS PIC XX.
       01 WS-CTRL-FS PIC XX.
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
       01 WS-MAX-SEQ PIC 9(6) VALUE 0.
       01 WS-KEY PIC X(8).
       01 WS-AMT PIC 9(9)V99 VALUE 0.
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
       01 ACCT-TABLE.
               05 ACCT-HOLD OCCURS 80 TIMES PIC X(33).
       01 PEND-TABLE.
               05 PEND-HOLD OCCURS 200 TIMES PIC X(65).
       01 NEW-TXN-TABLE.
               05 NEW-TXN OCCURS 400 TIMES PIC X(56).
       01 REJ-TABLE.
               05 REJ-HOLD OCCURS 200 TIMES PIC X(105).
       PROCEDURE DIVISION.
       MAIN-PARA.
               PERFORM LOAD-ACCOUNTS
               PERFORM SCAN-TXNS
               PERFORM LOAD-PENDING
               PERFORM VARYING WS-P FROM 1 BY 1
                   UNTIL WS-P > WS-PEND-COUNT
                   PERFORM APPLY-PENDING
               END-PERFORM
               PERFORM STORE-ACCOUNTS
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
      * Credits, including a transfer into type R, only
      * raise the cash balance. positions.dat is not updated.
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
