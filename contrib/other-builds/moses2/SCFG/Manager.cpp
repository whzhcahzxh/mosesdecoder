/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <vector>
#include <sstream>
#include "Manager.h"
#include "InputPath.h"
#include "Hypothesis.h"
#include "../PhraseBased/Sentence.h"
#include "../System.h"
#include "../TranslationModel/PhraseTable.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

Manager::Manager(System &sys, const TranslationTask &task,
    const std::string &inputStr, long translationId) :
    ManagerBase(sys, task, inputStr, translationId)
{

}

Manager::~Manager()
{

}

void Manager::Decode()
{
  // init pools etc
  //cerr << "START InitPools()" << endl;
  InitPools();
  //cerr << "START ParseInput()" << endl;
  ParseInput(true);

  size_t inputSize = GetInput().GetSize();
  //cerr << "size=" << size << endl;

  m_inputPaths.Init(GetInput(), *this);
  //cerr << "CREATED m_inputPaths" << endl;

  m_stacks.Init(*this, inputSize);
  //cerr << "CREATED m_stacks" << endl;

  for (int startPos = inputSize - 1; startPos >= 0; --startPos) {
    InitActiveChart(startPos);

    for (int phraseSize = 1; phraseSize < (inputSize - startPos + 1); ++phraseSize) {
      SubPhrase<Moses2::Word> sub = m_input->GetSubPhrase(startPos, phraseSize);
      //cerr << "sub=" << sub << endl;
      Lookup(startPos, phraseSize);
      Decode(startPos, phraseSize);
    }
  }

  m_stacks.OutputStacks();
}

void Manager::InitActiveChart(size_t pos)
{
   InputPath &path = *m_inputPaths.GetMatrix().GetValue(pos, 0);
   //cerr << "pos=" << pos << " path=" << path << endl;
   size_t numPt = system.mappings.size();
   //cerr << "numPt=" << numPt << endl;

   for (size_t i = 0; i < numPt; ++i) {
     const PhraseTable &pt = *system.mappings[i];
     //cerr << "START InitActiveChart" << endl;
     pt.InitActiveChart(path);
     //cerr << "FINISHED InitActiveChart" << endl;
   }
}

void Manager::Lookup(size_t startPos, size_t size)
{
  InputPath &path = *m_inputPaths.GetMatrix().GetValue(startPos, size);
  cerr << "path=" << path << endl;

  size_t numPt = system.mappings.size();
  //cerr << "numPt=" << numPt << endl;

  for (size_t i = 0; i < numPt; ++i) {
    const PhraseTable &pt = *system.mappings[i];
    pt.Lookup(GetPool(), *this, m_stacks, path);
  }

  /*
  size_t tpsNum = path.targetPhrases.GetSize();
  if (tpsNum) {
    cerr << tpsNum << " " << path << endl;
  }
  */
}

void Manager::Decode(size_t startPos, size_t size)
{
  //cerr << "size=" << size << " " << startPos << endl;

  InputPath &path = *m_inputPaths.GetMatrix().GetValue(startPos, size);
  Stack &stack = m_stacks.GetStack(startPos, size);

  Recycler<HypothesisBase*> &hypoRecycler = GetHypoRecycle();

  boost::unordered_map<SCFG::SymbolBind, SCFG::TargetPhrases>::const_iterator iterOuter;
  for (iterOuter = path.targetPhrases.begin(); iterOuter != path.targetPhrases.end(); ++iterOuter) {
    const SCFG::SymbolBind &symbolBind = iterOuter->first;

    const SCFG::TargetPhrases &tps = iterOuter->second;
    //cerr << "symbolBind=" << symbolBind << " " << tps.GetSize() << endl;

    SCFG::TargetPhrases::const_iterator iter;
    for (iter = tps.begin(); iter != tps.end(); ++iter) {
      const SCFG::TargetPhraseImpl &tp = **iter;
      SCFG::Hypothesis *hypo = new SCFG::Hypothesis(GetPool(), system);
      hypo->Init(*this, path, symbolBind, tp);

      StackAdd added = stack.Add(hypo, hypoRecycler, arcLists);
      //cerr << "added=" << added.added << " " << (const Phrase&) tp << endl;
    }
  }
}

}
}
