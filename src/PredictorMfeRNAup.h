
#ifndef PREDICTORMFERNAUP_H_
#define PREDICTORMFERNAUP_H_

#include "Predictor.h"
#include "Interaction.h"

#include <boost/numeric/ublas/matrix.hpp>

/**
 * Predictor for RNAup-like computation, i.e. full DP-implementation without
 * seed-heuristic
 *
 * @author Martin Mann
 *
 */
class PredictorMfeRNAup: public Predictor {

protected:

	//! matrix type to cover the energies for different interaction site widths
	typedef boost::numeric::ublas::matrix<E_type> E2dMatrix;

	//! full 4D DP-matrix for computation to hold all start position combinations
	//! first index = start positions (i1,i2) of (seq1,seq2)
	//! second index = interaction window sizes (w1,w2) or NULL if (i1,i2) not complementary
	typedef boost::numeric::ublas::matrix< E2dMatrix* > E4dMatrix;


public:

	/**
	 * Constructs a predictor and stores the energy and output handler
	 *
	 * @param energy the interaction energy handler
	 * @param output the output handler to report mfe interactions to
	 */
	PredictorMfeRNAup( const InteractionEnergy & energy, OutputHandler & output );

	virtual ~PredictorMfeRNAup();

	/**
	 * Computes the mfe for the given sequence ranges (i1-j1) in the first
	 * sequence and (i2-j2) in the second sequence and reports it to the output
	 * handler.
	 *
	 * @param r1 the index range of the first sequence interacting with r2
	 * @param 22 the index range of the second sequence interacting with r1
	 *
	 */
	virtual
	void
	predict( const IndexRange & r1 = IndexRange(0,RnaSequence::lastPos)
			, const IndexRange & r2 = IndexRange(0,RnaSequence::lastPos) );

protected:

	//! access to the interaction energy handler of the super class
	using Predictor::energy;

	//! access to the output handler of the super class
	using Predictor::output;

	//! energy of all interaction hybrids computed by the recursion with indices
	//! hybridE(i1,i2)->(w1,w2), with interaction start i1 (seq1) and i2 (seq2) and
	//! ineraction end j1=i1+w1 and j2=j2+w2
	//! NOTE: hybridE(i1,i2)==NULL if not complementary(seq1[i1],seq2[i2])
	E4dMatrix hybridE;

	//! mfe interaction boundaries
	Interaction mfeInteraction;

	//! offset for indices in sequence 1 for current computation
	size_t i1offset;
	//! offset for indices in sequence 2 for current computation
	size_t i2offset;

protected:

	/**
	 * Removes all temporary data structures and resets the predictor
	 */
	void
	clear();

	/**
	 * computes all entries of the hybridE matrix
	 */
	void
	fillHybridE( const InteractionEnergy & energy );

	/**
	 * Computes the final energy for an interaction given the hybridization
	 * energy
	 * @param hybridE the hybridization energy
	 * @param i1 the index of the first sequence interacting with i2
	 * @param j1 the index of the first sequence interacting with j2
	 * @param i2 the index of the second sequence interacting with i1
	 * @param j2 the index of the second sequence interacting with j1
	 * @return the overall energy = hybridE + ED1 + ED2 + EdangleLeft + EdangleRight
	 */
	E_type
	getE( const E_type hybridE, const size_t i1, const size_t j1, const size_t i2, const size_t j2 ) const;

	/**
	 * Fills a given interaction (boundaries given) with the according
	 * hybridizing base pairs.
	 * @param interaction IN/OUT the interaction to fill
	 */
	void
	traceBack( Interaction & interaction ) const;

	/**
	 * Initializes the global energy minimum
	 */
	virtual
	void
	initMfe();

	/**
	 * updates the global optimum to be the mfe interaction if needed
	 *
	 * @param i1 the index of the first sequence interacting with i2
	 * @param j1 the index of the first sequence interacting with j2
	 * @param i2 the index of the second sequence interacting with i1
	 * @param j2 the index of the second sequence interacting with j1
	 * @param energy the overall energy of the interaction
	 */
	virtual
	void
	updateMfe( const size_t i1, const size_t j1
			, const size_t i2, const size_t j2
			, const E_type energy );

};

#endif /* PREDICTORMFERNAUP_H_ */
