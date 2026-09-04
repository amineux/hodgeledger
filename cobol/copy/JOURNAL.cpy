>>SOURCE FORMAT FREE
*> Fixed-width journal record, 96 bytes. Matches data/SCHEMA.md and C++ journal_dat().
01 JOURNAL-REC.
   05 JE-ID         PIC 9(8).
   05 JE-DATE       PIC 9(8).
   05 JE-DIM        PIC 9.
   05 JE-FILL       PIC X.
   05 DEBIT-ACCT    PIC X(16).
   05 CREDIT-ACCT   PIC X(16).
   05 AMOUNT-CENTS  PIC 9(10).
   05 MEMO          PIC X(36).
