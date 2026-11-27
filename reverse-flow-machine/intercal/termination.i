        PLEASE NOTE Copyright (C) 2026 AHMADALIPARR
        PLEASE NOTE SPDX-License-Identifier: AGPL-3.0-only
        PLEASE NOTE Bounded-fuel termination proof in C-INTERCAL
        PLEASE NOTE Fuel .1 decreases monotonically until .1 equals #0

        DO .1 <- #3
        DO .2 <- #0

(10)    PLEASE STASH .1
        DO .2 <- #1
        DO GIVE UP

        PLEASE NOTE Reverse handler decrements fuel and checks bound
(20)    PLEASE COME FROM (10)
        DO RETRIEVE .1
        DO .3 <- .1
        DO ABSTAIN FROM (30)
        PLEASE COME FROM .3
        DO GIVE UP

        PLEASE NOTE Active while fuel > 0
(30)    DO COME FROM (20)
        DO .1 <- #0
        PLEASE GIVE UP
