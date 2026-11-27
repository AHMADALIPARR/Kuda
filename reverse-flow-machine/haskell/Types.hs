-- Copyright (C) 2026 AHMADALIPARR
-- SPDX-License-Identifier: AGPL-3.0-only
{-# LANGUAGE DataKinds #-}
{-# LANGUAGE GADTs #-}
{-# LANGUAGE KindSignatures #-}

module ReverseFlowTypes where

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
    { sourceLabel :: Int
    , targetLabel :: Int
    , transition  :: Transition a b
    }

data SomeEdge where
    SomeEdge :: ReverseEdge a b -> SomeEdge

data StateSnapshot = StateSnapshot
    { statePhase   :: Phase
    , stateFuel    :: Int
    , stateVersion :: Int
    } deriving (Eq, Show)

data TraceEntry = TraceEntry
    { traceSeq    :: Int
    , traceSource :: Int
    , traceTarget :: Int
    , tracePhase  :: Phase
    , traceFuel   :: Int
    } deriving (Eq, Show)

validateEdge :: ReverseEdge 'Pending 'Validated
validateEdge = ReverseEdge 5 10 Validate

commitEdge :: ReverseEdge 'Validated 'Committed
commitEdge = ReverseEdge 10 20 Commit

rejectEdge :: ReverseEdge 'Validated 'Rejected
rejectEdge = ReverseEdge 10 30 Reject
