/**
 * @file    ColorSelectionWidget.cpp
 * @brief
 * @author  Juan José García <juanjose.garcia@urjc.es>
 * @date    2015
 * @remarks Copyright (c) 2015 GMRV/URJC. All rights reserved.
 * Do not distribute without further notice.
 */

#include "ColorSelectionWidget.h"

#include <QPainter>
#include <QRect>
#include <QColor>
#include <QColorDialog>
#include <iostream>

ColorSelectionWidget::ColorSelectionWidget( QWidget* initParent )
  : QWidget( initParent )
  , _color( 0, 0, 0 )
{
  setSizePolicy( QSizePolicy(QSizePolicy::Fixed,
                             QSizePolicy::Fixed));
  setMinimumHeight( 30 );
  setMinimumWidth( 30 );
}

void ColorSelectionWidget::paintEvent( QPaintEvent* /*event*/ )
{
  QPainter painter( this );

  QRect rectSquare( 0, 0, this->width( ), this->height( ));
  painter.fillRect( rectSquare, _color );
}

void ColorSelectionWidget::mousePressEvent( QMouseEvent* /*event*/ )
{

  QColor newColor = QColorDialog::getColor( _color, this,
                                         "Choose new color",
                                         QColorDialog::DontUseNativeDialog);
  if ( newColor.isValid( ))
  {
    _color = newColor;
    emit this->colorChanged( _color );
  }
}

void ColorSelectionWidget::color( const QColor& color_ )
{
  _color = color_;
  emit this->colorChanged( _color );
}
