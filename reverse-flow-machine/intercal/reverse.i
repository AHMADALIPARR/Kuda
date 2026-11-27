        PLEASE NOTE Copyright (C) 2026 AHMADALIPARR
        PLEASE NOTE SPDX-License-Identifier: AGPL-3.0-only
        PLEASE NOTE Reverse-flow continuation with NEXT FROM, STASH, RETRIEVE
        PLEASE NOTE and computed COME FROM in C-INTERCAL

        DO .1 <- #10
        DO .2 <- #20
        DO .3 <- #0

        PLEASE NOTE Preserve .1 and .2 before reverse jump
        DO STASH .1 + .2

(100)   DO .3 <- #1
        DO .1 <- #99
        PLEASE GIVE UP

        PLEASE NOTE NEXT FROM pushes return address after (100)
(200)   PLEASE NEXT FROM (100)
        DO RETRIEVE .1 + .2
        DO .4 <- #25
        PLEASE RESUME #1

        PLEASE NOTE Computed COME FROM selects destination by .4
(300)   DO COME FROM .4
        DO .5 <- #77
        PLEASE GIVE UP
