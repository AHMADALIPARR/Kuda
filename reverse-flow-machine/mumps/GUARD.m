GUARD ; Guard predicate evaluation and ABSTAIN/REINSTATE control
; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
 ; Evaluates g(sigma) in {0,1} for R = {(s,d,g,tau)}
 Q
 ;
CHECK(ID,SRC,TGT) ; Return 1 iff edge (SRC -> TGT) is enabled for ID
 N REQ,PHASE,FUEL
 I $$ISABSTAINED(ID,TGT) Q 0
 S REQ=$G(^REVERSE(SRC,TGT))
 I REQ="" Q 0
 S PHASE=$$GETPHASE^STATE(ID)
 I PHASE'=REQ Q 0
 S FUEL=$$GETFUEL^STATE(ID)
 I FUEL'>0 Q 0
 Q 1
 ;
ADMISSIBLE(FROM,TO) ; Enforce Haskell-mirrored phase transition law
 I FROM="PENDING",TO="VALIDATED" Q 1
 I FROM="VALIDATED",TO="COMMITTED" Q 1
 I FROM="VALIDATED",TO="REJECTED" Q 1
 Q 0
 ;
ABSTAIN(ID,TGT) ; Disable reverse-flow target (INTERCAL ABSTAIN)
 S ^ABSTAIN(ID,TGT)=1
 Q 1
 ;
REINSTATE(ID,TGT) ; Restore reverse-flow target (INTERCAL REINSTATE)
 K ^ABSTAIN(ID,TGT)
 Q 1
 ;
ISABSTAINED(ID,TGT) ; Check if target is currently abstained
 Q +$G(^ABSTAIN(ID,TGT),0)
