-- Copyright (C) 2026 AHMADALIPARR
-- SPDX-License-Identifier: AGPL-3.0-only
{-# LANGUAGE DataKinds #-}
{-# LANGUAGE GADTs #-}
{-# LANGUAGE KindSignatures #-}

module ReverseFlowInvariants where

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

data StateSnapshot = StateSnapshot
    { invPhase   :: Phase
    , invFuel    :: Int
    , invVersion :: Int
    } deriving (Eq, Show)

data TraceEntry = TraceEntry
    { entrySeq    :: Int
    , entrySource :: Int
    , entryTarget :: Int
    , entryPhase  :: Phase
    , entryFuel   :: Int
    } deriving (Eq, Show)

decodePhase :: String -> Maybe Phase
decodePhase "PENDING"   = Just Pending
decodePhase "VALIDATED" = Just Validated
decodePhase "COMMITTED" = Just Committed
decodePhase "REJECTED"  = Just Rejected
decodePhase _           = Nothing

encodePhase :: Phase -> String
encodePhase Pending   = "PENDING"
encodePhase Validated = "VALIDATED"
encodePhase Committed = "COMMITTED"
encodePhase Rejected  = "REJECTED"

admissiblePair :: Phase -> Phase -> Bool
admissiblePair Pending   Validated = True
admissiblePair Validated Committed = True
admissiblePair Validated Rejected  = True
admissiblePair _         _         = False

-- Enforce |E(s, sigma)| <= 1
checkDeterminism :: [a] -> Bool
checkDeterminism []  = True
checkDeterminism [_] = True
checkDeterminism _   = False

-- Enforce f(sigma_{n+1}) < f(sigma_n) and nonnegativity
checkFuelDecrease :: Int -> Int -> Bool
checkFuelDecrease prevFuel nextFuel =
    prevFuel > 0 && nextFuel >= 0 && nextFuel < prevFuel

verifyTransition :: StateSnapshot -> StateSnapshot -> Bool
verifyTransition before after =
    admissiblePair (invPhase before) (invPhase after)
        && checkFuelDecrease (invFuel before) (invFuel after)
        && invVersion after == invVersion before + 1

verifyTrace :: Int -> [TraceEntry] -> Bool
verifyTrace initialFuel entries = go Pending initialFuel entries
  where
    go _ _ [] = True
    go curPhase curFuel (e : rest) =
        admissiblePair curPhase (entryPhase e)
            && checkFuelDecrease curFuel (entryFuel e)
            && go (entryPhase e) (entryFuel e) rest
