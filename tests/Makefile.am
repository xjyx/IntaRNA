
# ensure serial test build and run
AUTOMAKE_OPTIONS = serial-tests

# list of tests to run
TESTS = runApiTests


# the program to build
check_PROGRAMS = runApiTests 

# test sources
runApiTests_SOURCES =	\
					catch.hpp \
					testEasyLoggingSetup.icc \
					AccessibilityConstraint_test.cpp \
					AccessibilityFromStream_test.cpp \
					AccessibilityBasePair_test.cpp \
					HelixConstraint_test.cpp \
					HelixHandlerStackingOnly_test.cpp \
					HelixHandlerStackingOnlyIdxOffset_test.cpp \
					HelixHandlerStackingOnlySeed_test.cpp \
					HelixHandlerStackingOnlySeedIdxOffset_test.cpp \
					HelixHandlerUnpaired_test.cpp \
					HelixHandlerUnpairedIdxOffset_test.cpp \
					HelixHandlerUnpairedSeed_test.cpp \
					HelixHandlerUnpairedSeedIdxOffset_test.cpp \
					IndexRange_test.cpp  \
					IndexRangeList_test.cpp  \
					Interaction_test.cpp  \
					InteractionEnergyBasePair_test.cpp  \
					InteractionRange_test.cpp  \
					PredictionTrackerProfileMinE_test.cpp \
					PredictorMfe2dHelixHeuristic_test.cpp \
					PredictorMfe2dHelixHeuristicSeed_test.cpp \
					PredictionTrackerSpotProb_test.cpp \
					NussinovHandler_test.cpp \
					RnaSequence_test.cpp \
					OutputHandlerRangeOnly_test.cpp \
					OutputHandlerInteractionList_test.cpp \
					SeedHandlerExplicit_test.cpp \
					SeedHandlerNoBulge_test.cpp \
					SeedHandlerMfe_test.cpp \
					SeedHandlerIdxOffset_test.cpp \
					runTests.cpp


# add IntaRNA lib for linking
LIBS= 	-L$(top_builddir)/src/IntaRNA -lIntaRNA \
		-L$(top_builddir)/src -leasylogging \
		@LIBS@

# add source include for compilation
runApiTests_CXXFLAGS =	-I$(top_builddir)/src \
						@AM_CXXFLAGS@ @CXXFLAGS@ \
						-DELPP_NO_LOG_TO_FILE
