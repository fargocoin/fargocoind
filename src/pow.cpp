// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The FargoCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
	bool fPoS = pindexLast->nHeight >= Params().LastPOWBlock();

	const CBlockIndex *BlockLastSolved = pindexLast;
	const CBlockIndex *BlockReading = pindexLast;
	int64_t nActualTimespan = 0;
	int64_t LastBlockTime = 0;
	int64_t PastBlocksMin = 24;
	int64_t PastBlocksMax = 24;
	int64_t CountBlocks = 0;
	arith_uint256 PastDifficultyAverage;
	arith_uint256 PastDifficultyAveragePrev;

	if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
	// This is the first block or the height is < PastBlocksMin
	// Return minimal required work. (1e0fffff)
		return UintToArith256(Params().GetConsensus().powLimit).GetCompact();
	}
	// loop over the past n blocks, where n == PastBlocksMax
	for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
		if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
		CountBlocks++;

	// Calculate average difficulty based on the blocks we iterate over in this for loop
	if(CountBlocks <= PastBlocksMin) {
		if (CountBlocks == 1) { PastDifficultyAverage.SetCompact(BlockReading->nBits); }
	else { PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks)+(arith_uint256().SetCompact(BlockReading->nBits))) / (CountBlocks+1); }
		PastDifficultyAveragePrev = PastDifficultyAverage;
	}

	// If this is the second iteration (LastBlockTime was set)
	if(LastBlockTime > 0){
	// Calculate time difference between previous block and current block
		int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
	// Increment the actual timespan
		nActualTimespan += Diff;
	}
	// Set LasBlockTime to the block time for the block in current iteration
	LastBlockTime = BlockReading->GetBlockTime();

	if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
		BlockReading = BlockReading->pprev;
	}
	// bnNew is the difficulty
	arith_uint256 bnNew(PastDifficultyAverage);

	// nTargetTimespan is the time that the CountBlocks should have taken to be generated.
	int64_t nTargetTimespan = CountBlocks*(Params().GetConsensus().nPowTargetSpacing);

	// Limit the re-adjustment to 3x or 0.33x
	// We don't want to increase/decrease diff too much.
	if (nActualTimespan < nTargetTimespan/3)
		nActualTimespan = nTargetTimespan/3;
	if (nActualTimespan > nTargetTimespan*3)
		nActualTimespan = nTargetTimespan*3;

	// Calculate the new difficulty based on actual and target timespan.
	bnNew *= nActualTimespan;
	bnNew /= nTargetTimespan;

	// If calculated difficulty is lower than the minimal diff, set the new difficulty to be the minimal diff.
	arith_uint256 limit = fPoS ?
			UintToArith256(Params().GetConsensus().posLimit) :
			UintToArith256(Params().GetConsensus().powLimit);
	if (bnNew > limit){
		bnNew = limit;
	}
	// Some logging.
	// TODO: only display these log messages for a certain debug option.
	// Return the new diff.
	return bnNew.GetCompact();
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("params.nPowTargetTimespan = %d    nActualTimespan = %d\n", params.nPowTargetTimespan, nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return error("CheckProofOfWork(): nBits below minimum work");

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return error("CheckProofOfWork(): hash doesn't match nBits");

    return true;
}

arith_uint256 GetBlockProof(const CBlockIndex& block)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork) {
        r = to.nChainWork - from.nChainWork;
    } else {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(params.nPowTargetSpacing) / GetBlockProof(tip);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}
