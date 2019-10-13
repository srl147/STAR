/*
* Created by Fahimeh Mirhaj on 5/30/19.
*/  

#include "GTF.h"
#include "streamFuns.h"

void GTF::superTranscript() {
    if (P.pGe.gTypeString!="Transcriptome" && P.pGe.gTypeString!="SuperTranscriptome")
        return;
    
    for(uint64 i = 0; i < exonLoci.size(); i++) {//convert (-)strand exons coordinates
        uint64 transId = exonLoci[i][exT];
        if(transcriptStrand[transId] == 2) {
            uint64 temp = exonLoci[i][exS];
            exonLoci[i][exS] = 2 * genome.nGenome - 1 - exonLoci[i][exE];
            exonLoci[i][exE] = 2 * genome.nGenome - 1 - temp;
        }
    }
    
    //sort by exon start position
    sort(exonLoci.begin(), exonLoci.end(),
         [](const array<uint64, exL>& e1, const array<uint64, exL>& e2) {
             return e1[exS] < e2[exS];
         });
    
    uint64 gapValue = exonLoci[0][exS];
    array<uint64,2> curr = {exonLoci[0][exS], exonLoci[0][exE]}; //intervals[0]
    exonLoci[0][exS] = 0;
    exonLoci[0][exE] -= gapValue;
    vector<array<uint64,2>> mergedIntervals;
    
    for(uint64 i = 1; i < exonLoci.size(); ++i) {
        if(exonLoci[i][exS] <= curr[1]+1) {
            curr[1] = max(curr[1], exonLoci[i][exE]);
        } else {
            gapValue += exonLoci[i][exS] - curr[1] - 1;
            mergedIntervals.push_back(curr);
            curr = {exonLoci[i][exS], exonLoci[i][exE]};
        };
        exonLoci[i][exS] -= gapValue;
        exonLoci[i][exE] -= gapValue;
    };
    mergedIntervals.push_back(curr);
    
    //Condensed genome (a.k.a SuperTranscriptome) sequence
    for(const auto& p: mergedIntervals) {
        for(uint64 j = p[0]; j <= p[1]; ++j) {
            superTr.seqConcat.push_back((uint8) genome.G[j]);
        };
    };
    P.inOut->logMain << "SuperTranscriptome (condensed) genome length = " << superTr.seqConcat.size() <<endl;
    
    //Sort rows of exonLoci based on transId and startInterval
    //Find super and normal transcript intervals and sequences
    transcriptStartEnd.resize(transcriptID.size(), {(uint64)-1, 0}); //normal transcript
    for(uint64 i = 0 ; i < exonLoci.size(); i++) {//start-end of the normal transcripts in the condensed genome coordinates
        uint64 transId = exonLoci[i][exT];
        transcriptStartEnd[transId].first = min(transcriptStartEnd[transId].first, exonLoci[i][exS]);
        transcriptStartEnd[transId].second = max(transcriptStartEnd[transId].second, exonLoci[i][exE]);
    };
    
    vector<pair<uint64, uint64>> tempTranscriptIntervals = transcriptStartEnd;//sorted transript intervals
    sort(tempTranscriptIntervals.begin(), tempTranscriptIntervals.end(),
         [](const pair<uint64, uint64>& t1, const pair<uint64, uint64>& t2) {
             return t1.first < t2.first;
         });
    curr = tempTranscriptIntervals.front();
    for(const auto& p: tempTranscriptIntervals) {//if transcript intervals overlap by >=1 base, merge them into SuperTranscripts
        if(p.first <= curr.second) { //without +1
            curr.second = max(curr.second, p.second);
        } else {
            superTr.startEndInFullGenome.push_back(curr);
            curr = p;
        };
    };
    superTr.startEndInFullGenome.push_back(curr);//record last element

    //superTranscript sequences
    uint64 maxSTlen=0;
    for(const auto& p: superTr.startEndInFullGenome) {
        superTr.seq.push_back(vector<uint8>(superTr.seqConcat.begin()+p.first, superTr.seqConcat.begin()+p.second));
        maxSTlen=max(maxSTlen,p.second-p.first);
    };
    P.inOut->logMain << "Number of superTranscripts = " << superTr.startEndInFullGenome.size() <<";   max length = " <<maxSTlen <<endl;
    
    superTr.trIndex.resize(transcriptID.size());
    //for each normal transcript, find superTranscript it belongs to
    uint64 ist=0;
    for (auto &ee: exonLoci) {
        if (ee[exS]>superTr.startEndInFullGenome[ist].second)
            ++ist;
        superTr.trIndex[ee[exT]]=ist;
    };
    
    
    superTr.trStartEnd.resize(transcriptID.size());
    for(uint i = 0; i < transcriptStartEnd.size(); i++) {
        superTr.trStartEnd[i].first = transcriptStartEnd[i].first - superTr.startEndInFullGenome[superTr.trIndex[i]].first;
        superTr.trStartEnd[i].second = transcriptStartEnd[i].second - superTr.startEndInFullGenome[superTr.trIndex[i]].first;
    }
    //////////////////Normal transcripts
    sort(exonLoci.begin(), exonLoci.end(),
         [](const array<uint64, exL>& e1, const array<uint64, exL>& e2) {
             return (e1[exT] < e2[exT]) || ((e1[exT] == e2[exT]) && (e1[exS] < e2[exS]));
         });
    
//     ofstream & trInfoOut = ofstrOpen(P.pGe.gDir+"/transcriptChrStartEndSJs.tsv",ERROR_OUT, P);
//     uint64 trIndPrev=exonLoci[0][exT]];
    transcriptSeq.resize(transcriptID.size());         
    for(uint64 i = 0; i < exonLoci.size(); i++) {
        transcriptSeq[exonLoci[i][exT]].insert(transcriptSeq[exonLoci[i][exT]].end(),
                                                             superTr.seqConcat.begin()+exonLoci[i][exS], superTr.seqConcat.begin()+exonLoci[i][exE] + 1);
//         
//         if (exonLoci[i][exT]] != trIndPrev) {
//             trIndPrev=exonLoci[i][exT]];
//             trInfoOut <<'\n'<< trID[trIndPrev] <<"\t"<< trChr[trIndPrev];
//         };
//          trInfoOut <<"\t"<<exonLoci[i][exS]
        
    };
//    trInfoOut.close();

    //output normal transcript sequences
    vector<char> numToCharConverter{'A','C','G','T','N','M'};
    ofstream & trSeqOut = ofstrOpen(P.pGe.gDir+"/transcriptSequences.fasta",ERROR_OUT, P);
    for(uint64 i = 0; i < transcriptSeq.size(); i++) {
        trSeqOut << ">" << transcriptID[i] << "\n";
        for(uint64 j = 0; j < transcriptSeq[i].size(); j++) {
            trSeqOut << numToCharConverter[transcriptSeq[i][j]];
        }
        trSeqOut << "\n";
    }
    trSeqOut.close();
    
    // splice junctions, in superTranscript coordinates. Here, sjS is the last base of donor exon, and sjE is the last base of acceptor exon
    for(uint64 i = 1; i < exonLoci.size(); i++) {//exonLoci are still sorted by transcript index and start coordinate
        if (exonLoci[i][exT]==exonLoci[i-1][exT]) {//same transcript
            if (exonLoci[i][exS] > (exonLoci[i-1][exE]+1)) {//gap >0, otherwise we do not need to record the junction (may need it in the future for other purposes though)
                uint64 sti=superTr.trIndex[exonLoci[i][exT]];//ST index
                uint64 sts=superTr.startEndInFullGenome[sti].first; //superTranscript start
                superTr.sj.emplace_back();//add one element
                superTr.sj.back().start = (uint32)(exonLoci[i-1][exE]-sts);//start at the last base of the exon
                superTr.sj.back().end = (uint32)(exonLoci[i]  [exS]-sts);//end at the first base of the next exon
                superTr.sj.back().tr = exonLoci[i][exT];//the transcript info is retained, thus there will be duplicate junctions that belong to different transcripts
                superTr.sj.back().super = sti;
            };
        };
    };
    superTr.sjCollapse();
    
    //replace the Genome sequence and all necessary parameters for downstream genome generation
    if (P.pGe.gTypeString=="Transcriptome") {
        genome.concatenateChromosomes(transcriptSeq, transcriptID, genome.genomeChrBinNbases);
        gtfYes=false; //annotations were converted into transcript sequences and are no longer used
        P.sjdbInsert.yes=false; //actually, it might pe possible to add splice junctions on top of the transcriptome
    } else if (P.pGe.gTypeString=="SuperTranscriptome") {
        vector<string> superTranscriptID(superTr.startEndInFullGenome.size());
        for (uint64 ii=0; ii<superTranscriptID.size(); ii++)
            superTranscriptID[ii]="st" + to_string(ii);
        
        genome.concatenateChromosomes(superTr.seq, superTranscriptID, genome.genomeChrBinNbases);
        
        transcriptStrand.resize(superTr.startEndInFullGenome.size(),1);//transcript are always on + strand in superTr
        
        //sort but exon start position
        sort(exonLoci.begin(), exonLoci.end(),
            [](const array<uint64, exL>& e1, const array<uint64, exL>& e2)
            {
                return e1[exS] < e2[exS];
            }
        );
        //shift exonLoci according to chrStart
        uint64 ist=0;
        for (auto &ee: exonLoci) {
            if (ee[exS]>superTr.startEndInFullGenome[ist].second)
                ++ist;
            ee[exS]+=genome.chrStart[ist]-superTr.startEndInFullGenome[ist].first;
            ee[exE]+=genome.chrStart[ist]-superTr.startEndInFullGenome[ist].first;
        };
    };
}

void Genome::concatenateChromosomes(const vector<vector<uint8>> &vecSeq, const vector<string> &vecName, const uint64 padBin)
{//concatenate chromosome sequence into linear genome, with padding
 //changes genome: chrStart, chrLen, chrName, G  
    nChrReal=vecSeq.size();
    chrLength.resize(nChrReal,0);
    chrStart.resize(nChrReal+1,0);
    chrName=vecName;
    chrNameIndex.clear();
    for (uint32 ii=0; ii<nChrReal; ii++) {
        chrLength[ii]=vecSeq[ii].size();
        chrStart[ii+1]=chrStart[ii]+((chrLength[ii]+1)/padBin+1)*padBin;//+1 makes sure that there is at least one spacer base between chromosomes
        chrNameIndex[chrName[ii]]=ii;
    };

    nGenome=chrStart.back();
    //re-allocate G and copy the sequences
    delete[] G1;
    genomeSequenceAllocate();
    for (uint32 ii=0; ii<nChrReal; ii++) {
        memcpy(G+chrStart[ii],vecSeq[ii].data(),vecSeq[ii].size());
    };
    
    for (uint ii=0;ii<nGenome;ii++) {//- strand
        G[2*nGenome-1-ii]=G[ii]<4 ? 3-G[ii] : G[ii];
    };    
};
