/***************************************************************************
                          ailexer.cpp  -  description
                             -------------------
    begin                : Wed Jul 18 2001
    copyright            : (C) 2001 by Dirk Schönberger
    email                : schoenberger@signsoft.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ailexer.h"

#define CATEGORY_WHITESPACE -1
#define CATEGORY_ALPHA -2
#define CATEGORY_DIGIT -3
#define CATEGORY_SPECIAL -4

#define CATEGORY_ANY -127

#define STOP 0

int iswhitespace(char c){
  return (c==' ')||(c=='\n')||(c=='\t');
}

int isspecial(char c){
  return (c=='*')/* ||(c=='/') */ ||(c=='_');
}

const char*statetoa (State state){
  switch (state)
  {
    case State_Comment : return "comment";
    case State_Integer : return "integer";
    case State_Float : return "float";
    case State_String : return "string";
    case State_Token : return "token";
    case State_Reference : return "reference";
    case State_Start : return "start";
    case State_BlockStart : return "block start";
    case State_BlockEnd : return "block end";
    case State_ArrayStart : return "array start";
    case State_ArrayEnd : return "array end";
    default : return "unknown";
  }
}

typedef struct {
  State oldState;
  char c;
  State newState;
  Action action;
} Transition;

static Transition transitions[] = {
  { State_Start, '%', State_Comment, Action_Ignore},
  { State_Start, CATEGORY_DIGIT, State_Integer, Action_Copy},
  { State_Start, '-', State_Integer, Action_Copy},
  { State_Start, '+', State_Integer, Action_Copy},
  { State_Start, '.', State_Float, Action_Copy},
  { State_Start, '/', State_Reference, Action_Ignore },
  { State_Start, CATEGORY_ALPHA, State_Token, Action_Copy},
  { State_Start, CATEGORY_SPECIAL, State_Token, Action_Copy},
  { State_Start, '(', State_String, Action_Ignore},
  { State_Start, CATEGORY_WHITESPACE, State_Start, Action_Output},
  { State_Start, '{', State_BlockStart, Action_Copy},
  { State_Start, '}', State_BlockEnd, Action_Copy},
  { State_Start, '[', State_ArrayStart, Action_Copy},
  { State_Start, ']', State_ArrayEnd, Action_Copy},
  { State_Start, CATEGORY_ANY, State_Start, Action_Abort},
//  { State_Array, CATEGORY_ALPHA, State_Array, Action_Copy},
//  { State_Array, CATEGORY_DIGIT, State_Array, Action_Copy},
//  { State_Array, ' ', State_Array, Action_Copy},
  { State_Comment, '\n', State_Start, Action_Output},
  { State_Comment, CATEGORY_ANY, State_Comment, Action_Copy},
  { State_Integer, CATEGORY_DIGIT, State_Integer, Action_Copy},
  { State_Integer, CATEGORY_WHITESPACE, State_Start, Action_Output},
  { State_Integer, ']', State_Start, Action_OutputUnget},
  { State_Integer, '.', State_Float, Action_Copy},
  { State_Integer, CATEGORY_ANY, State_Start, Action_Abort},
  { State_Float, CATEGORY_DIGIT, State_Float, Action_Copy},
  { State_Float, CATEGORY_WHITESPACE, State_Start, Action_Output},
  { State_Float, CATEGORY_ANY, State_Start, Action_Abort},
  { State_String, ')', State_Start, Action_Output},
  { State_String, CATEGORY_ANY, State_String, Action_Copy},
  { State_Token, CATEGORY_DIGIT, State_Token, Action_Copy},
  { State_Token, CATEGORY_ALPHA, State_Token, Action_Copy},
  { State_Token, CATEGORY_SPECIAL, State_Token, Action_Copy},
  { State_Token, CATEGORY_WHITESPACE, State_Start, Action_Output},
  { State_Token, '{', State_BlockStart, Action_Output},
  { State_Token, '}', State_BlockEnd, Action_Output},
  { State_Token, CATEGORY_ANY, State_Start, Action_Abort},
  { State_BlockStart, CATEGORY_ANY, State_Start, Action_OutputUnget },
  { State_BlockEnd, CATEGORY_ANY, State_Start, Action_OutputUnget },
  { State_ArrayStart, CATEGORY_ANY, State_Start, Action_OutputUnget },
  { State_ArrayEnd, CATEGORY_ANY, State_Start, Action_OutputUnget },
  { State_Reference, CATEGORY_ALPHA, State_Reference, Action_Copy },
  { State_Reference, CATEGORY_DIGIT, State_Reference, Action_Copy },
  { State_Reference, CATEGORY_ANY, State_Start, Action_OutputUnget },
  { State_Start, STOP, State_Start, Action_Abort}
};

AILexer::AILexer(){
}
AILexer::~AILexer(){
}

bool AILexer::parse (QIODevice& fin){
  char c;

  m_buffer = "";
  m_curState = State_Start;

  parsingStarted();

  while (!fin.atEnd())
  {
    c = fin.getch ();

    State newState;
    Action action;

    nextStep (c, &newState, &action);

    switch (action)
    {
      case Action_Copy :
        m_buffer += c;
        break;
      case Action_CopyOutput :
        m_buffer += c;
        doOutput();
        break;
      case Action_Output :
        doOutput();
        break;
      case Action_OutputUnget :
        doOutput();
        fin.ungetch(c);
        break;
      case Action_Ignore :
        /* ignore */
        break;
      case Action_Abort :
        qWarning ( "state %s / %s char %c (%d)" , statetoa(m_curState), statetoa(newState), c, c );
        parsingAborted();
        return false;
        break;
      default :
        qWarning ( "unknown action: %d ", action);
    }

    m_curState = newState;
  }

  parsingFinished();
  return true;
}

void AILexer::doOutput ()
{
  switch (m_curState)
  {
    case State_Comment :
      gotComment (m_buffer.latin1());
      break;
    case State_Integer :
      gotIntValue (m_buffer.toInt());
      break;
    case State_Float :
      gotDoubleValue (m_buffer.toFloat());
      break;
    case State_String :
      gotStringValue (m_buffer.latin1());
      break;
    case State_Token :
      gotToken (m_buffer.latin1());
      break;
    case State_Reference :
      gotReference (m_buffer.latin1());
      break;
    case State_BlockStart :
      gotBlockStart ();
      break;
    case State_BlockEnd :
      gotBlockEnd ();
      break;
    case State_Start :
      break;
    case State_ArrayStart :
      gotArrayStart ();
      break;
    case State_ArrayEnd :
      gotArrayEnd ();
      break;
    default:
      qWarning ( "unknown state: %d", m_curState );
  }

  m_buffer = "";
}

void AILexer::gotComment (const char *value) {
  qDebug ( "gotComment: %s ", value );
}

void AILexer::gotIntValue (int value) {
  qDebug ( "gotInt: %d ", value );
}

void AILexer::gotDoubleValue (double value) {
  qDebug ( "gotDouble: %f ", value );
}

void AILexer::gotStringValue (const char *value) {
  qDebug ( "gotString: %s ", value );
}

void AILexer::gotToken (const char *value) {
  qDebug ( "gotToken: %s ", value );
}

void AILexer::gotReference (const char *value) {
  qDebug ( "gotReference: %s ", value );
}

void AILexer::gotBlockStart (){
  qDebug ( "gotBlockStart" );
}

void AILexer::gotBlockEnd (){
  qDebug ( "gotBlockEnd" );
}

void AILexer::gotArrayStart (){
  qDebug ( "gotArrayStart" );
}

void AILexer::gotArrayEnd (){
  qDebug ( "gotArrayEnd" );
}

void AILexer::parsingStarted() {
  qDebug ( "parsing started" );
}

void AILexer::parsingFinished() {
  qDebug ( "parsing finished" );
}

void AILexer::parsingAborted() {
  qDebug ( "parsing aborted" );
}

void AILexer::nextStep (char c, State *newState, Action *newAction) {
  int i=0;

  while (true) {
    Transition trans = transitions[i];

    if (trans.c == STOP) {
      *newState = trans.newState;
      *newAction = trans.action;
      return;
    }

    bool found = false;

    QChar ch(c);
    if (trans.oldState == m_curState) {
      switch (trans.c) {
        case CATEGORY_WHITESPACE : found = ch.isSpace(); break;
        case CATEGORY_ALPHA : found = ch.isLetter(); break;
        case CATEGORY_DIGIT : found = ch.isNumber(); break;
        case CATEGORY_SPECIAL : found = isspecial(c); break;
        case CATEGORY_ANY : found = true; break;
        default : found = (trans.c == c);
      }

      if (found) {
        *newState = trans.newState;
        *newAction = trans.action;

        return;
      }
    }


    i++;
  }
}


