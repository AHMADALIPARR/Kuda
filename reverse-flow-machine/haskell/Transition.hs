-- Copyright (C) 2026 AHMADALIPARR
-- SPDX-License-Identifier: AGPL-3.0-only
{-# LANGUAGE DataKinds #-}
{-# LANGUAGE GADTs #-}
{-# LANGUAGE KindSignatures #-}

module ReverseFlowTransition where

-- Original Haskell syntax only: zero imports, zero external dependencies.

data Phase
    = Pending
    | Validated
    | Committed
    | Rejected
    deriving (Eq, Show)

data Transition (a :: Phase) (b :: Phase) where
    Validate :: Transition 'Pending 'Validated
    Commit   :: Transition 'Validated 'Committed
    Reject   :: Transition 'Validated 'Rejected

data ReverseEdge (a :: Phase) (b :: Phase) = ReverseEdge
    { edgeSource     :: Int
    , edgeTarget     :: Int
    , edgeTransition :: Transition a b
    }

data MachineState = MachineState
    { msPhase     :: Phase
    , msFuel      :: Int
    , msVersion   :: Int
    , msAbstained :: [Int]
    } deriving (Eq, Show)

targetPhase :: Transition a b -> Phase
targetPhase Validate = Validated
targetPhase Commit   = Committed
targetPhase Reject   = Rejected

sourcePhase :: Transition a b -> Phase
sourcePhase Validate = Pending
sourcePhase Commit   = Validated
sourcePhase Reject   = Validated

isAbstained :: Int -> MachineState -> Bool
isAbstained lbl st = elem lbl (msAbstained st)

abstainLabel :: Int -> MachineState -> MachineState
abstainLabel lbl st
    | isAbstained lbl st = st
    | otherwise          = st { msAbstained = lbl : msAbstained st }

reinstateLabel :: Int -> MachineState -> MachineState
reinstateLabel lbl st =
    st { msAbstained = filter (/= lbl) (msAbstained st) }

applyTransition :: ReverseEdge a b -> MachineState -> Either String MachineState
applyTransition edge st
    | isAbstained (edgeTarget edge) st =
        Left "Target label is abstained"
    | msFuel st <= 0 =
        Left "Termination fuel exhausted"
    | msPhase st /= sourcePhase (edgeTransition edge) =
        Left "Phase guard mismatch"
    | otherwise =
        Right st
            { msPhase   = targetPhase (edgeTransition edge)
            , msFuel    = msFuel st - 1
            , msVersion = msVersion st + 1
            }
