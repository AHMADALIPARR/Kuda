TRACE ; Deterministic transaction ledger and replay verification
; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
 ; Stores ^TRACE(ID,SEQ,"SOURCE"|"TARGET"|"PHASE"|"FUEL")
 Q
 ;
RECORD(ID,SRC,TGT,PHASE,FUEL) ; Append transition record to ledger
 N SEQ
 S SEQ=$I(^TRACE(ID))
 S ^TRACE(ID,SEQ,"SOURCE")=SRC
 S ^TRACE(ID,SEQ,"TARGET")=TGT
 S ^TRACE(ID,SEQ,"PHASE")=PHASE
 S ^TRACE(ID,SEQ,"FUEL")=FUEL
 Q SEQ
 ;
COUNT(ID) ; Return number of recorded transitions
 Q +$G(^TRACE(ID),0)
 ;
REPLAY(ID,OUTPHASE,OUTFUEL) ; Deterministically replay ledger from initial state
 N SEQ,MAX,SRC,TGT,PH,FL,PREVFL,OK
 S MAX=$$COUNT(ID),OK=1,PREVFL=999999999
 S OUTPHASE="PENDING",OUTFUEL=0
 F SEQ=1:1:MAX D  Q:'OK
 . S SRC=$G(^TRACE(ID,SEQ,"SOURCE"))
 . S TGT=$G(^TRACE(ID,SEQ,"TARGET"))
 . S PH=$G(^TRACE(ID,SEQ,"PHASE"))
 . S FL=+$G(^TRACE(ID,SEQ,"FUEL"))
 . I '$$ADMISSIBLE^GUARD(OUTPHASE,PH) S OK=0 Q
 . I SEQ>1,FL'<PREVFL S OK=0 Q
 . S OUTPHASE=PH,OUTFUEL=FL,PREVFL=FL
 Q OK
 ;
DUMP(ID) ; Print human-readable ledger to current device
 N SEQ,MAX
 S MAX=$$COUNT(ID)
 F SEQ=1:1:MAX D
 . W SEQ,": ",^TRACE(ID,SEQ,"SOURCE")," -> ",^TRACE(ID,SEQ,"TARGET")
 . W " [",^TRACE(ID,SEQ,"PHASE"),", fuel=",^TRACE(ID,SEQ,"FUEL"),"]",!
 Q
