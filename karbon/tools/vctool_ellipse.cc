/* This file is part of the KDE project
   Copyright (C) 2001, The Karbon Developers
*/

#include "vccmd_ellipse.h"	// command
#include "vcdlg_ellipse.h"	// dialog
#include "vctool_ellipse.h"

VCToolEllipse* VCToolEllipse::s_instance = 0L;

VCToolEllipse::VCToolEllipse( KarbonPart* part )
	: m_part( part ), m_isDragging( false ), m_isSquare( false ),
	  m_isCentered( false )
{
	// create config dialog:
	m_dialog = new VCDlgEllipse();
	m_dialog->setValueWidth( 100.0 );
	m_dialog->setValueHeight( 100.0 );
}

VCToolEllipse::~VCToolEllipse()
{
	delete( m_dialog );
}

VCToolEllipse*
VCToolEllipse::instance( KarbonPart* part )
{
	if ( s_instance == 0L )
	{
		s_instance = new VCToolEllipse( part );
	}

	return s_instance;
}

bool
VCToolEllipse::eventFilter( KarbonView* view, QEvent* event )
{

	if ( event->type() == QEvent::MouseMove && m_isDragging )
	{
		// erase old object:
		drawTemporaryObject( view );

		QMouseEvent* mouse_event = static_cast<QMouseEvent*> ( event );
		m_lp  = view->canvasWidget()->viewportToContents( mouse_event->pos() );

		recalcCoords();

		// paint new object:
		drawTemporaryObject( view );

		return true;
	}

	if ( event->type() == QEvent::MouseButtonRelease && m_isDragging )
	{
		m_isDragging = false;
		m_isSquare = false;
		m_isCentered = false;

		QMouseEvent* mouse_event = static_cast<QMouseEvent*> ( event );
		m_lp  = view->canvasWidget()->viewportToContents( mouse_event->pos() );

		// did we drag the mouse?
		if ( m_fp == m_lp )
		{
			// we didnt drag => show a config-dialog:
			if ( m_dialog->exec() )
			{
				m_part->addCommand(
					new VCCmdEllipse( m_part,
						m_fp.x(), m_fp.y(),
						m_fp.x() + m_dialog->valueWidth(),
						m_fp.y() + m_dialog->valueHeight() ) );
			}
			else
				// erase old object:
				drawTemporaryObject( view );
		}
		else
		{
			m_part->addCommand(
				new VCCmdEllipse( m_part, m_tl.x(), m_tl.y(), m_br.x(), m_br.y() ) );
		}

		return true;
	}

	// handle pressing of keys:
	if ( event->type() == QEvent::KeyPress )
	{
		QKeyEvent* key_event = static_cast<QKeyEvent*> ( event );

		// cancel dragging with ESC-key:
		if ( key_event->key() == Qt::Key_Escape && m_isDragging )
		{
			m_isDragging = false;
			m_isSquare = false;
			m_isCentered = false;

			// erase old object:
			drawTemporaryObject( view );

			return true;
		}

		// if SHIFT is pressed, we want a square:
		if ( key_event->key() == Qt::Key_Shift )
		{
			m_isSquare = true;

			if ( m_isDragging )
			{
				// erase old object:
				drawTemporaryObject( view );
				recalcCoords();
				// draw new old object:
				drawTemporaryObject( view );
			}

			return true;
		}

		// if Ctrl is pressed, we want a centered path:
		if ( key_event->key() == Qt::Key_Control )
		{
			m_isCentered = true;

			if ( m_isDragging )
			{
				// erase old object:
				drawTemporaryObject( view );
				recalcCoords();
				// draw new old object:
				drawTemporaryObject( view );
			}

			return true;
		}
	}

	// handle releasing of keys:
	if ( event->type() == QEvent::KeyRelease )
	{
		QKeyEvent* key_event = static_cast<QKeyEvent*> ( event );

		if ( key_event->key() == Qt::Key_Shift )
		{
			m_isSquare = false;

			if ( m_isDragging )
			{
				// erase old object:
				drawTemporaryObject( view );
				recalcCoords();
				// draw new old object:
				drawTemporaryObject( view );
			}

			return true;
		}

		if ( key_event->key() == Qt::Key_Control )
		{
			m_isCentered = false;

			if ( m_isDragging )
			{
				// erase old object:
				drawTemporaryObject( view );
				recalcCoords();
				// draw new old object:
				drawTemporaryObject( view );
			}

			return true;
		}
	}

	// the whole story starts with this event:
	if ( event->type() == QEvent::MouseButtonPress )
	{
		QMouseEvent* mouse_event = static_cast<QMouseEvent*> ( event );
		m_fp = view->canvasWidget()->viewportToContents( mouse_event->pos() );
		m_lp = view->canvasWidget()->viewportToContents( mouse_event->pos() );
		
		// draw initial object:
		drawTemporaryObject( view );

		m_isDragging = true;

		return true;
	}

	return false;
}


