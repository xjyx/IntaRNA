
#include "PredictorMfeRNAup.h"

#include <stdexcept>


////////////////////////////////////////////////////////////////////////////

PredictorMfeRNAup::
PredictorMfeRNAup( const InteractionEnergy & energy, OutputHandler & output )
 : Predictor(energy,output)
	, hybridE( 0,0 )
	, mfeInteraction(energy.getAccessibility1().getSequence()
			,energy.getAccessibility2().getAccessibilityOrigin().getSequence())
	, i1offset(0)
	, i2offset(0)
{
}


////////////////////////////////////////////////////////////////////////////

PredictorMfeRNAup::
~PredictorMfeRNAup()
{
	// clean up
	this->clear();
}


////////////////////////////////////////////////////////////////////////////

void
PredictorMfeRNAup::
predict( const IndexRange & r1
		, const IndexRange & r2 )
{
#ifdef NDEBUG
	// no check
#else
	// check indices
	if (!(r1.isAscending() && r2.isAscending()) )
		throw std::runtime_error("PredictorMfeRNAup::predict("+toString(r1)+","+toString(r2)+") is not sane");
#endif

	// clear data
	clear();

	// resize matrix
	hybridE.resize( std::min( energy.getAccessibility1().getSequence().size()
						, (r1.to==RnaSequence::lastPos?energy.getAccessibility1().getSequence().size()-1:r1.to)-r1.from+1 )
				, std::min( energy.getAccessibility2().getSequence().size()
						, (r2.to==RnaSequence::lastPos?energy.getAccessibility2().getSequence().size()-1:r2.to)-r2.from+1 ) );

	i1offset = r1.from;
	i2offset = r2.from;

	size_t debug_count_cells_null=0
			, debug_count_cells_nonNull = 0
			, debug_count_cells_inf = 0
			, debug_cellNumber=0
			, w1, w2;

	// get best possible energy contributions to skip certain cell computations
	const E_type bestStackingEnergy = energy.getBestStackingEnergy();
	const E_type bestInitEnergy = energy.getBestInitEnergy();
	const E_type bestDangleEnergy = energy.getBestDangleEnergy();

	bool i1blocked, i1or2blocked, skipw1w2;
	// initialize 3rd and 4th dimension of the matrix
	for (size_t i1=0; i1<hybridE.size1(); i1++) {
		// check if i1 is blocked for interaction
		i1blocked =
		// - shows ambiguous nucleotide encoding
			energy.getAccessibility1().getSequence().isAmbiguous( i1+i1offset )
		// - is blocked by an accessibility constraint
			|| energy.getAccessibility1().getAccConstraint().isBlocked(i1+i1offset);
	for (size_t i2=0; i2<hybridE.size2(); i2++) {
		// check whether i1 or i2 is blocked for interaction
		i1or2blocked = i1blocked
		// - shows ambiguous nucleotide encoding
			|| energy.getAccessibility2().getSequence().isAmbiguous( i2+i2offset )
		// - is blocked by an accessibility constraint
			|| energy.getAccessibility2().getAccConstraint().isBlocked(i2+i2offset);

		debug_cellNumber =
				/*w1 = */ std::min(energy.getAccessibility1().getMaxLength(), hybridE.size1()-i1)
			*	/*w2 = */ std::min(energy.getAccessibility2().getMaxLength(), hybridE.size2()-i2);

		// check if i1 and i2 are not blocked and can form a base pair
		if ( ! i1or2blocked
			&& RnaSequence::areComplementary(
				  energy.getAccessibility1().getSequence()
				, energy.getAccessibility2().getSequence()
				, i1+i1offset, i2+i2offset ))
		{
			// create new 2d matrix for different interaction site widths
			hybridE(i1,i2) = new E2dMatrix(
				/*w1 = */ std::min(energy.getAccessibility1().getMaxLength(), hybridE.size1()-i1),
				/*w2 = */ std::min(energy.getAccessibility2().getMaxLength(), hybridE.size2()-i2));

			debug_count_cells_nonNull += debug_cellNumber;

			// screen for cells that can be skipped from computation
			for (size_t w1x = (*hybridE(i1,i2)).size1(); w1x>0; w1x--) {
				w1 = w1x-1;

			for (size_t w2x = (*hybridE(i1,i2)).size2(); w2x>0; w2x--) {
				w2 = w2x-1;

				// check if window size too large
				skipw1w2 = 1+(w1*(energy.getMaxInternalLoopSize1()+1)) < w2
						|| 1+(w2*(energy.getMaxInternalLoopSize2()+1)) < w1;

				// check if ED penalty exceeds maximal energy gain
				if (!skipw1w2) {
					// check if all larger windows are already set to INF
					bool largerWindowsINF = w1x==(*hybridE(i1,i2)).size1() && w2x==(*hybridE(i1,i2)).size2();
					// check all larger windows (that might need this window for computation)
					for (size_t w1p=w1+1; largerWindowsINF && w1p<(*hybridE(i1,i2)).size1(); w1p++) {
					for (size_t w2p=w2+1; largerWindowsINF && w2p<(*hybridE(i1,i2)).size2(); w2p++) {
						// check if larger window is E_INF
						largerWindowsINF = (std::numeric_limits<E_type>::max() < (*hybridE(i1,i2))(w1p,w2p));
					}
					}

					// if it holds for all w'>=w: ED1(i1+w1')+ED2(i2+w2')+dangle > -1*(min(w1',w2')*EmaxStacking + Einit)
					// ie. the ED values exceed the max possible energy gain of an interaction
					skipw1w2 = skipw1w2
							|| ( largerWindowsINF &&
									( -1.0*(std::min(w1,w2)*bestStackingEnergy + bestInitEnergy + bestDangleEnergy) >
										(energy.getAccessibility1().getED(i1+i1offset,i1+w1+i1offset)
												+ energy.getAccessibility2().getED(i2+i2offset,i2+w2+i2offset)))
								)
								;
				}

				if (skipw1w2) {
					// init with infinity to mark that this cell is not to be computed later on
					(*hybridE(i1,i2))(w1,w2) = E_INF;
					debug_count_cells_inf++;
				}

			}
			}

		} else {
			// reduce memory consumption and avoid computation for this start index combination
			hybridE(i1,i2) = NULL;
			debug_count_cells_null += debug_cellNumber;
		}
	}
	}

	std::cerr <<"#DEBUG: init 4d matrix : "<<(debug_count_cells_nonNull-debug_count_cells_inf)<<" (-"<<debug_count_cells_inf <<") to be filled ("
				<<((double)(debug_count_cells_nonNull-debug_count_cells_inf)/(double)(debug_count_cells_nonNull+debug_count_cells_null))
				<<"%) and "<<debug_count_cells_null <<" not allocated ("
				<<((double)(debug_count_cells_null)/(double)(debug_count_cells_nonNull+debug_count_cells_null))
				<<"%)"
				<<std::endl;

	// fill matrix
	fillHybridE( energy );

	// check if interaction is better than no interaction (E==0)
	if (mfeInteraction.energy < 0.0) {
		// fill mfe interaction with according base pairs
		traceBack( mfeInteraction );
	} else {
		// TODO : check if better to skip output handler report instead of overwrite
		// replace mfeInteraction with no interaction
		mfeInteraction.clear();
		mfeInteraction.energy = 0.0;
	}

	// report mfe interaction
	output.add( mfeInteraction );
}

////////////////////////////////////////////////////////////////////////////

void
PredictorMfeRNAup::
clear()
{
	// delete 3rd and 4th dimension of the matrix
	for (E4dMatrix::iterator1 ijEntry = hybridE.begin1(); ijEntry != hybridE.end1(); ijEntry++) {
		if (*ijEntry != NULL) {
			// delete 2d matrix for current ij
			delete (*ijEntry);
			*ijEntry = NULL;
		}
	}
	// clear matrix, free data
	hybridE.clear();
}

////////////////////////////////////////////////////////////////////////////

void
PredictorMfeRNAup::
fillHybridE( const InteractionEnergy & energy )
{

	// global vars to avoid reallocation
	size_t i1,i2,j1,j2,w1,w2,k1,k2;

	//////////  FIRST ROUND : COMPUTE HYBRIDIZATION ENERGIES ONLY  ////////////

	// current minimal value
	E_type curMinE = E_INF;
	// iterate increasingly over all window sizes w1 (seq1) and w2 (seq2)
	for (w1=0; w1<energy.getAccessibility1().getMaxLength(); w1++) {
	for (w2=0; w2<energy.getAccessibility2().getMaxLength(); w2++) {
		// iterate over all window starts i (seq1) and k (seq2)
		// TODO PARALLELIZE THIS DOUBLE LOOP ?!
		for (i1=0; i1+w1<hybridE.size1(); i1++) {
		for (i2=0; i2+w2<hybridE.size2(); i2++) {
			// check if left boundary is complementary
			if (hybridE(i1,i2) == NULL) {
				// nothing to do
				continue;
			}
			// get window ends j (seq1) and l (seq2)
			j1=i1+w1;
			j2=i2+w2;
			// check if right boundary is complementary
			if (hybridE(j1,j2) == NULL)
			{
				// not complementary -> ignore this entry
				(*hybridE(i1,i2))(w1,w2) = E_INF;
			} else {

				// check if this cell is to be computed (!=E_INF)
				if( (*hybridE(i1,i2))(w1,w2) < std::numeric_limits<E_type>::infinity()) {

					// compute entry
					// get full internal loop energy (nothing between i and j)
					curMinE = energy.getInterLoopE(i1+i1offset,j1+i1offset,i2+i2offset,j2+i2offset) + energy.getInterLoopE(j1+i1offset,j1+i1offset,j2+i2offset,j2+i2offset);


					if (w1 > 1 && w2 > 1) {
						// check all combinations of decompositions into (i1,i2)..(k1,k2)-(j1,j2)
						for (k1=std::min(j1-1,i1+energy.getMaxInternalLoopSize1()+1); k1>i1; k1--) {
						for (k2=std::min(j2-1,i2+energy.getMaxInternalLoopSize2()+1); k2>i2; k2--) {
							// check if (k1,k2) are complementary
							if (hybridE(k1,k2) != NULL) {
								curMinE = std::min( curMinE,
										(energy.getInterLoopE(i1+i1offset,k1+i1offset,i2+i2offset,k2+i2offset) + (*hybridE(k1,k2))(j1-k1,j2-k2))
										);
							}
						}
						}
					}
					// store value
					(*hybridE(i1,i2))(w1,w2) = curMinE;

				}
			}
		}
		}
	}
	}

	//////////  SECOND ROUND : COMPUTE FINAL ENERGIES AND MFE  ////////////

	// initialize mfe interaction for updates
	initMfe();

	// iterate increasingly over all window sizes w1 (seq1) and w2 (seq2)
	for (w1=0; w1<energy.getAccessibility1().getMaxLength(); w1++) {
	for (w2=0; w2<energy.getAccessibility2().getMaxLength(); w2++) {
		// iterate over all window starts i (seq1) and k (seq2)
		for (i1=0; i1+w1<hybridE.size1(); i1++) {
		for (i2=0; i2+w2<hybridE.size2(); i2++) {
			// check if left boundary is complementary
			if (hybridE(i1,i2) == NULL) {
				// nothing to do
				continue;
			}
			// check if reasonable entry
			if ((*hybridE(i1,i2))(w1,w2) < E_INF) {
				// get window ends j (seq1) and l (seq2)
				j1=i1+w1;
				j2=i2+w2;

				// update mfe if needed
				updateMfe( i1,j1,i2,j2
						, getE( (*hybridE(i1,i2))(w1,w2), i1,j1,i2,j2)
						);

			}

		} // i2
		} // i1
	} // w2
	} // w1

}


////////////////////////////////////////////////////////////////////////////

E_type
PredictorMfeRNAup::
getE( const E_type hybridE, const size_t i1, const size_t j1, const size_t i2, const size_t j2 ) const
{
	return hybridE
			+ energy.getDanglingLeft(i1+i1offset,i2+i2offset)
			+ energy.getDanglingRight(j1+i1offset,j2+i2offset)
			+ energy.getAccessibility1().getED(i1+i1offset,j1+i1offset)
			+ energy.getAccessibility2().getED(i2+i2offset,j2+i2offset)
			;
}

////////////////////////////////////////////////////////////////////////////

void
PredictorMfeRNAup::
initMfe()
{
	// initialize global E minimum
	mfeInteraction.energy = E_INF;
	// ensure it holds only the boundary
	if (mfeInteraction.basePairs.size()!=2) {
		mfeInteraction.basePairs.resize(2);
	}
	// reset boundary base pairs
	mfeInteraction.basePairs[0].first = RnaSequence::lastPos;
	mfeInteraction.basePairs[0].second = RnaSequence::lastPos;
	mfeInteraction.basePairs[1].first = RnaSequence::lastPos;
	mfeInteraction.basePairs[1].second = RnaSequence::lastPos;
}
////////////////////////////////////////////////////////////////////////////

void
PredictorMfeRNAup::
updateMfe( const size_t i1, const size_t j1
		, const size_t i2, const size_t j2
		, const E_type curE )
{
//				std::cerr <<"#DEBUG : energy( "<<i1<<"-"<<j1<<", "<<i2<<"-"<<j2<<" ) = "
//						<<ecurE
//						<<" = " <<(eH + eE + eD)
//						<<std::endl;

	if (curE < mfeInteraction.energy) {
		// store new global min
		mfeInteraction.energy = (curE);
		// store interaction boundaries
		// left
		mfeInteraction.basePairs[0].first = i1+i1offset;
		mfeInteraction.basePairs[0].second = energy.getAccessibility2().getReversedIndex(i2+i2offset);
		// right
		mfeInteraction.basePairs[1].first = j1+i1offset;
		mfeInteraction.basePairs[1].second = energy.getAccessibility2().getReversedIndex(j2+i2offset);
	}
}

////////////////////////////////////////////////////////////////////////////

void
PredictorMfeRNAup::
traceBack( Interaction & interaction ) const
{
	// check if something to trace
	if (interaction.basePairs.size() < 2) {
		return;
	}

#ifdef NDEBUG           /* required by ANSI standard */
	// no check
#else
	// sanity checks
	if ( ! interaction.isValid() ) {
		throw std::runtime_error("PredictorRNAup::traceBack() : given interaction not valid");
	}
	if ( interaction.basePairs.size() != 2 ) {
		throw std::runtime_error("PredictorRNAup::traceBack() : given interaction does not contain boundaries only");
	}
#endif

	// check for single interaction
	if (interaction.basePairs.at(0).first == interaction.basePairs.at(1).first) {
		// delete second boundary (identical to first)
		interaction.basePairs.resize(1);
		// update done
		return;
	}

	// ensure sorting
	interaction.sort();
	// get indices in hybridE for boundary base pairs
	size_t	i1 = interaction.basePairs.at(0).first - i1offset,
			j1 = interaction.basePairs.at(1).first - i1offset,
			i2 = energy.getAccessibility2().getReversedIndex(interaction.basePairs.at(0).second - i2offset),
			j2 = energy.getAccessibility2().getReversedIndex(interaction.basePairs.at(1).second - i2offset);

	// the currently traced value for i1-j1, i2-j2
	E_type curE = (*hybridE(i1,i2))(j1-i1,j2-i2);

	// trace back
	do {
		// check if just internal loop
		if (curE == (energy.getInterLoopE(i1+i1offset,j1+i1offset,i2+i2offset,j2+i2offset) + energy.getInterLoopE(j1+i1offset,j1+i1offset,j2+i2offset,j2+i2offset))) {
			break;
		}
		// check all interval splits
		if ( (j1-i1) > 1 && (j2-i2) > 1) {
			// temp variables
			size_t k1,k2;
			bool traceNotFound = true;
			// check all combinations of decompositions into (i1,i2)..(k1,k2)-(j1,j2)
			for (k1=std::min(j1-1,i1+energy.getMaxInternalLoopSize1()+1); traceNotFound && k1>i1; k1--) {
			for (k2=std::min(j2-1,i2+energy.getMaxInternalLoopSize2()+1); traceNotFound && k2>i2; k2--) {
				// check if (k1,k2) are complementary
				if (hybridE(k1,k2) != NULL) {
					if (curE == (energy.getInterLoopE(i1+i1offset,k1+i1offset,i2+i2offset,k2+i2offset) + (*hybridE(k1,k2))(j1-k1,j2-k2)) ) {
						// stop searching
						traceNotFound = false;
						// store splitting base pair
						interaction.addInteraction( k1+i1offset, energy.getAccessibility2().getReversedIndex(k2+i2offset) );
						// trace right part of split
						i1=k1;
						i2=k2;
						curE = (*hybridE(i1,i2))(j1-i1,j2-i2);
					}
				}
			}
			}
		}
	// do until only right boundary is left over
	} while( i1 != j1 );

	// sort final interaction (to make valid) (faster than calling sort())
	if (interaction.basePairs.size() > 2) {
		Interaction::PairingVec & bps = interaction.basePairs;
		// shift all added base pairs to the front
		for (size_t i=2; i<bps.size(); i++) {
			bps.at(i-1).first = bps.at(i).first;
			bps.at(i-1).second = bps.at(i).second;
		}
		// set last to j1-j2
		bps.rbegin()->first = j1+i1offset;
		bps.rbegin()->second = energy.getAccessibility2().getReversedIndex(j2+i2offset);
	}

}

////////////////////////////////////////////////////////////////////////////

