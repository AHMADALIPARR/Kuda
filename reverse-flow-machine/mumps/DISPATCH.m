DISPATCH ; Indexed transition registry and deterministic reverse-flow dispatch
; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
 ; Enforces |E(s,sigma)| <= 1, sigma_{n+1}=tau(sigma_n), f(sigma_{n+1}) < f(sigma_n)
 Q
 ;
REGISTER(SRC,TGT,REQPHASE,NEXTPHASE) ; Register reverse edge in ^REVERSE
 I '$$ADMISSIBLE^GUARD(REQPHASE,NEXTPHASE) Q 0
 S ^REVERSE(SRC,TGT)=REQPHASE
 S ^REVERSE(SRC,TGT,"NEXT")=NEXTPHASE
 Q 1
 ;
ENABLED(ID,SRC,OUTTGT) ; Compute E(s,sigma) and enforce |E(s,sigma)| <= 1
 N TGT,CNT
 S TGT="",CNT=0,OUTTGT=""
 F  S TGT=$O(^REVERSE(SRC,TGT)) Q:TGT=""  D
 . I $$CHECK^GUARD(ID,SRC,TGT) D
 . . S CNT=CNT+1
 . . S OUTTGT=TGT
 I CNT>1 Q -1 ; Ambiguous reverse-flow dispatch rejected
 I CNT=0 Q 0  ; No enabled edge
 Q 1
 ;
STEP(ID,SRC) ; Execute one deterministic atomic reverse transition
 N ST,TGT,REQ,NEXT,OK
 S ST=$$ENABLED(ID,SRC,.TGT)
 I ST'=1 Q ST
 S REQ=$G(^REVERSE(SRC,TGT))
 S NEXT=$G(^REVERSE(SRC,TGT,"NEXT"))
 I '$$ADMISSIBLE^GUARD(REQ,NEXT) Q 0
 TS
 S OK=$$SETPHASE^STATE(ID,REQ,NEXT)
 I 'OK TRO  Q 0
 S OK=$$DECFUEL^STATE(ID)
 I 'OK TRO  Q 0
 D RECORD^TRACE(ID,SRC,TGT,NEXT,$$GETFUEL^STATE(ID))
 TC
 Q TGT
 ;
RUN(ID,START,MAXSTEPS) ; Bounded execution loop from START label
 N CUR,STEPS,RES
 S CUR=START,STEPS=0
 F  Q:STEPS'<MAXSTEPS  D  Q:RES'>0
 . S RES=$$STEP(ID,CUR)
 . I RES>0 S CUR=RES,STEPS=STEPS+1
 Q CUR
