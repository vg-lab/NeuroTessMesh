/**
 * @file    ColorSelectionWidget.h
 * @brief
 * @author  Juan José García <juanjose.garcia@urjc.es>
 * @date    2015
 * @remarks Copyright (c) 2015 VG-Lab/URJC. All rights reserved.
 * Do not distribute without further notice.
 */

#ifndef __NEUROTESSMESH_COLOR_SELECTION_WIDGET__
#define __NEUROTESSMESH_COLOR_SELECTION_WIDGET__

#include <QWidget>
#include <QColor>

class ColorSelectionWidget: public QWidget
{

  Q_OBJECT

public:

  ColorSelectionWidget( QWidget* parent = nullptr );

  void paintEvent( QPaintEvent* e );

  void mousePressEvent( QMouseEvent* e );

  void paint( QPainter* painter );

  void color( const QColor& color_ );

signals:
  void colorChanged( QColor color );

private:

  QColor _color;

};

#endif // __NEUROTESSMESH_COLOR_SELECTION_WIDGET__
