-- Copyright (C) 2026 AHMADALIPARR
-- SPDX-License-Identifier: AGPL-3.0-only
{-# LANGUAGE DataKinds #-}
{-# LANGUAGE GADTs #-}
{-# LANGUAGE KindSignatures #-}

module ReverseFlowCompiler where

-- Original Haskell syntax only: zero imports, zero external dependencies.
-- Static lowering compiler from MUMPS-style COME_FROM macros to MUMPS globals.

data MacroDirective
    = ReqPhase String
    | ReqFuelPositive
    | TransPhase String String
    | DecFuel
    | RecordEdge Int Int
    deriving (Eq, Show)

data MacroBlock = MacroBlock
    { mbTarget     :: Int
    , mbSource     :: Int
    , mbDirectives :: [MacroDirective]
    } deriving (Eq, Show)

admissibleMacro :: String -> String -> Bool
admissibleMacro "PENDING"   "VALIDATED" = True
admissibleMacro "VALIDATED" "COMMITTED" = True
admissibleMacro "VALIDATED" "REJECTED"  = True
admissibleMacro _           _           = False

lowerBlock :: MacroBlock -> Either String [String]
lowerBlock blk =
    case findTransition (mbDirectives blk) of
        Nothing -> Left "Missing TRANSITION directive"
        Just (fromPh, toPh)
            | not (admissibleMacro fromPh toPh) ->
                Left ("Inadmissible transition: " ++ fromPh ++ " -> " ++ toPh)
            | otherwise ->
                Right
                    [ "S ^REVERSE(" ++ show (mbSource blk) ++ "," ++ show (mbTarget blk) ++ ")=\"" ++ fromPh ++ "\""
                    , "S ^REVERSE(" ++ show (mbSource blk) ++ "," ++ show (mbTarget blk) ++ ",\"NEXT\")=\"" ++ toPh ++ "\""
                    , "S OK=$$STEP^DISPATCH(ID," ++ show (mbSource blk) ++ ")"
                    ]

findTransition :: [MacroDirective] -> Maybe (String, String)
findTransition [] = Nothing
findTransition (TransPhase a b : _) = Just (a, b)
findTransition (_ : xs) = findTransition xs

compileProgram :: [MacroBlock] -> Either String String
compileProgram [] = Right ""
compileProgram (b : bs) = do
    linesOut <- lowerBlock b
    restOut  <- compileProgram bs
    Right (unlines linesOut ++ restOut)

exampleCommitBlock :: MacroBlock
exampleCommitBlock = MacroBlock
    { mbTarget = 20
    , mbSource = 10
    , mbDirectives =
        [ ReqPhase "VALIDATED"
        , ReqFuelPositive
        , TransPhase "VALIDATED" "COMMITTED"
        , DecFuel
        , RecordEdge 10 20
        ]
    }
