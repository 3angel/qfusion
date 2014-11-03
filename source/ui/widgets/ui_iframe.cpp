/*
Copyright (C) 2014 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_main.h"
#include "widgets/ui_widgets.h"
#include "widgets/ui_idiv.h"
#include "formatters/ui_colorcode_formatter.h"

namespace WSWUI {

using namespace Rocket::Core;

class IFrameWidget : public Element
{
public:
	IFrameWidget( const String &tag ) : Element(tag)
	{
		SetProperty( "display", "inline-block" );
		SetProperty( "overflow", "auto" );
	}

	virtual ~IFrameWidget()
	{}
	
	// Called when attributes on the element are changed.
	void OnAttributeChange( const Rocket::Core::AttributeNameList& changed_attributes )
	{
		Element::OnAttributeChange(changed_attributes);

		AttributeNameList::const_iterator it;

		// Check for a changed 'src' attribute. If this changes, we need to reload
		// contents of the element.
		it = changed_attributes.find( "src" );
		if( it != changed_attributes.end() ) {
			LoadSource();
		}
	}

private:
	void LoadSource()
	{
		String source = GetAttribute< String >("src", "");

		SetInnerRML( "" );

		if( source.Empty() ) {
			return;
		}

		WSWUI::NavigationStack *stack = UI_Main::Get()->createStack();
		if( stack == NULL ) {
			return;
		}
		WSWUI::Document *ui_document = stack->pushDocument( source.CString() );
		if( !ui_document ) {
			return;
		}

		ElementDocument *document = ui_document->getRocketDocument();
		AppendChild( document );
		document->SetProperty( "overflow", "auto" );
		document->PullToFront();
	}
};

//==============================================================

ElementInstancer *GetIFrameWidgetInstancer( void )
{
	return __new__( GenericElementInstancer<IFrameWidget> )();
}

}
