STATE ; Persistent state machine for reverse-flow VM (YottaDB / MUMPS)
; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
 ; Uses ^STATE(ID,"PHASE"), ^STATE(ID,"FUEL"), ^STATE(ID,"VERSION")
 Q
 ;
INIT(ID,FUEL) ; Initialize machine state inside a transaction
 N OK
 S OK=1
 I $G(ID)="" Q 0
 I +$G(FUEL)<0 Q 0
 TS
 K ^STATE(ID),^TRACE(ID),^ABSTAIN(ID)
 S ^STATE(ID,"PHASE")="PENDING"
 S ^STATE(ID,"FUEL")=+$G(FUEL)
 S ^STATE(ID,"VERSION")=0
 TC
 Q OK
 ;
GETPHASE(ID) ; Return current phase string or "" if uninitialized
 Q $G(^STATE(ID,"PHASE"))
 ;
GETFUEL(ID) ; Return current termination budget (nonnegative integer)
 Q +$G(^STATE(ID,"FUEL"),-1)
 ;
GETVER(ID) ; Return monotonic state version
 Q +$G(^STATE(ID,"VERSION"),0)
 ;
SETPHASE(ID,FROM,TO) ; Atomic phase transition with optimistic check
 N CUR
 S CUR=$G(^STATE(ID,"PHASE"))
 I CUR'=FROM Q 0
 S ^STATE(ID,"PHASE")=TO
 S ^STATE(ID,"VERSION")=$G(^STATE(ID,"VERSION"),0)+1
 Q 1
 ;
DECFUEL(ID) ; Decrement fuel strictly: f(sigma_{n+1}) < f(sigma_n)
 N F
 S F=+$G(^STATE(ID,"FUEL"),0)
 I F'>0 Q 0
 S ^STATE(ID,"FUEL")=F-1
 Q 1
 ;
RESET(ID) ; Clear machine state and trace
 K ^STATE(ID),^TRACE(ID),^ABSTAIN(ID)
 Q 1
