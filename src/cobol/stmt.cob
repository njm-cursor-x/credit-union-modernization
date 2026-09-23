       IDENTIFICATION DIVISION.
       PROGRAM-ID. STMT.
       ENVIRONMENT DIVISION.
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT MEMBER-FILE ASSIGN TO "data/members.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-MBR-FS.
           SELECT ACCOUNT-FILE ASSIGN TO "data/accounts.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-ACCT-FS.
           SELECT TXN-FILE ASSIGN TO "data/txns.dat"
               ORGANIZATION IS LINE SEQUENTIAL
               FILE STATUS IS WS-TXN-FS.
       DATA DIVISION.
       FILE SECTION.
       FD MEMBER-FILE.
       01 MEMBER-REC PIC X(150).
       FD ACCOUNT-FILE.
       01 ACCOUNT-REC PIC X(33).
       FD TXN-FILE.
       01 TXN-REC PIC X(56).
       WORKING-STORAGE SECTION.
       01 WS-MBR-FS PIC XX.
       01 WS-ACCT-FS PIC XX.
       01 WS-TXN-FS PIC XX.
       01 WS-EOF PIC X VALUE "N".
       01 WS-FOUND PIC X VALUE "N".
       01 WS-MBR-IN PIC X(6).
       01 WS-LINE PIC X(72).
       01 WS-MONEY PIC ZZZ,ZZZ,ZZ9.99.
       01 WS-FMT-DATE PIC X(8).
       01 MBR-WORK.
               COPY "member.cpy".
       01 ACCT-WORK.
               COPY "account.cpy".
       01 TXN-WORK.
               COPY "txn.cpy".
       PROCEDURE DIVISION.
       MAIN-PARA.
               ACCEPT WS-MBR-IN
               PERFORM FIND-MEMBER
               IF WS-FOUND NOT = "Y"
                   DISPLAY "MEMBER NOT FOUND"
                   STOP RUN RETURNING 1
               END-IF
               PERFORM PRINT-HEAD
               PERFORM PRINT-ACCOUNTS
               STOP RUN.
       FIND-MEMBER.
               OPEN INPUT MEMBER-FILE
               IF WS-MBR-FS NOT = "00"
                   DISPLAY "MEMBER OPEN " WS-MBR-FS
                   STOP RUN RETURNING 1
               END-IF
               MOVE "N" TO WS-EOF
               MOVE "N" TO WS-FOUND
               PERFORM UNTIL WS-EOF = "Y" OR WS-FOUND = "Y"
                   READ MEMBER-FILE
                       AT END
                           MOVE "Y" TO WS-EOF
                       NOT AT END
                           MOVE MEMBER-REC TO MBR-WORK
                           IF MBR-NUM = WS-MBR-IN
                               MOVE "Y" TO WS-FOUND
                           END-IF
                   END-READ
               END-PERFORM
               CLOSE MEMBER-FILE.
       PRINT-HEAD.
               DISPLAY "LAMEX CREDIT UNION"
               DISPLAY "MEMBER STATEMENT"
               DISPLAY " "
               MOVE SPACES TO WS-LINE
               STRING "MEMBER " MBR-NUM "  " MBR-NAME
                   DELIMITED BY SIZE INTO WS-LINE
               END-STRING
               DISPLAY WS-LINE
               MOVE SPACES TO WS-LINE
               STRING "SSN    " MBR-SSN
                   DELIMITED BY SIZE INTO WS-LINE
               END-STRING
               DISPLAY WS-LINE
               DISPLAY " ".
       PRINT-ACCOUNTS.
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
                           MOVE ACCOUNT-REC TO ACCT-WORK
                           IF ACCT-MBR = WS-MBR-IN
                               PERFORM PRINT-ONE-ACCT
                           END-IF
                   END-READ
               END-PERFORM
               CLOSE ACCOUNT-FILE.
       PRINT-ONE-ACCT.
               DISPLAY " "
               MOVE SPACES TO WS-LINE
               STRING "ACCOUNT " ACCT-NUM "  TYPE " ACCT-TYPE
                   "  STATUS " ACCT-STATUS
                   DELIMITED BY SIZE INTO WS-LINE
               END-STRING
               DISPLAY WS-LINE
               IF ACCT-TYPE = "R"
                   DISPLAY "RETIREMENT MARKET VALUE"
                   DISPLAY "CALL THE BRANCH TO CHANGE INVESTMENTS"
               END-IF
               DISPLAY "DATE      DESCRIPTION               AMOUNT"
               PERFORM PRINT-TXNS
               MOVE "N" TO WS-EOF
               MOVE ACCT-BAL TO WS-MONEY
               MOVE SPACES TO WS-LINE
               STRING "BALANCE " WS-MONEY
                   DELIMITED BY SIZE INTO WS-LINE
               END-STRING
               DISPLAY WS-LINE.
       PRINT-TXNS.
               OPEN INPUT TXN-FILE
               MOVE "N" TO WS-EOF
               PERFORM UNTIL WS-EOF = "Y"
                   READ TXN-FILE
                       AT END
                           MOVE "Y" TO WS-EOF
                       NOT AT END
                           MOVE TXN-REC TO TXN-WORK
                           IF TXN-ACCT = ACCT-NUM
                               PERFORM PRINT-ONE-TXN
                           END-IF
                   END-READ
               END-PERFORM
               CLOSE TXN-FILE.
       PRINT-ONE-TXN.
               MOVE TXN-DATE TO WS-FMT-DATE
               MOVE TXN-DATE (3:2) TO WS-FMT-DATE (1:2)
               MOVE "/" TO WS-FMT-DATE (3:1)
               MOVE TXN-DATE (5:2) TO WS-FMT-DATE (4:2)
               MOVE "/" TO WS-FMT-DATE (6:1)
               MOVE TXN-DATE (1:2) TO WS-FMT-DATE (7:2)
               MOVE TXN-AMT TO WS-MONEY
               MOVE SPACES TO WS-LINE
               STRING WS-FMT-DATE " " TXN-DESC " " WS-MONEY
                   " " TXN-DC
                   DELIMITED BY SIZE INTO WS-LINE
               END-STRING
               DISPLAY WS-LINE.
