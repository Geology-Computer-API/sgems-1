/**********************************************************************
** Author: Nicolas Remy
** Copyright (C) 2002-2004 The Board of Trustees of the Leland Stanford Junior
**   University
** All rights reserved.
**
** This file is part of the "gui" module of the Geostatistical Earth
** Modeling Software (GEMS)
**
** This file may be distributed and/or modified under the terms of the 
** license defined by the Stanford Center for Reservoir Forecasting and 
** appearing in the file LICENSE.XFREE included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.gnu.org/copyleft/gpl.html for GPL licensing information.
**
** Contact the Stanford Center for Reservoir Forecasting, Stanford University
** if any conditions of this licensing are not clear to you.
**
**********************************************************************/

#include <GsTLAppli/gui/variogram2/variog_plot.h>
#include <GsTLAppli/math/discrete_function.h>
#include <GsTL/math/math_functions.h>
#include <GsTLAppli/math/gstlpoint.h>
#include <GsTL/geometry/geometry_algorithms.h>

#include <qpen.h>
#include <qbrush.h>
#include <QVector>
#include <qpainter.h>

#include <QMouseEvent>

#include <algorithm>
#include <cmath>

#include <GsTLAppli/extra/qwt/qwt_symbol.h>
#include <GsTLAppli/extra/qwt/qwt_scale_engine.h>
#include <GsTLAppli/extra/qwt/qwt_scale_div.h>
#include <GsTLAppli/extra/qwt/qwt_plot_curve.h>
#include <GsTLAppli/extra/qwt/qwt_plot_marker.h>
#include <GsTLAppli/extra/qwt/qwt_plot_grid.h>
#include <GsTLAppli/extra/qwt/qwt_legend.h>
#include <QIcon>

Variog_plot::
Variog_plot( const Discrete_function& df,
			       const Variogram_function_adaptor<Covariance<GsTLPoint> > *model,
			       GsTLVector<double> angle,
             const std::vector<int>& pairs,
             const QString& title, bool y_starts_at_zero,
			       QWidget *parent, const char *name)
  : QwtPlot(parent), pairs_shown_(false), model1_(0), model2_(0)
{

  //this->setWindowFlags( );
  setWindowFlags(Qt::SubWindow | Qt::MSWindowsFixedSizeDialogHint);
  
  if (name)
    setObjectName(name);

  setWindowIcon(QIcon());
  
  //------------
  // configure the plotting area

  //use a light gray for the grid
  QwtPlotGrid * grid = new QwtPlotGrid();
  grid->enableX(true);
  grid->setPen( QPen( QColor(150,150,150), 0, Qt::DotLine ) );
  grid->attach(this);

  this->axisScaleEngine(QwtPlot::xBottom)->setReference(0);
  this->axisScaleEngine(QwtPlot::yLeft)->setReference(0);
  this->axisScaleEngine(QwtPlot::xBottom)->setMargins(0,0.5);
  this->axisScaleEngine(QwtPlot::yLeft)->setMargins(0,0.5);
  this->axisScaleEngine(QwtPlot::xBottom)->setAttributes(QwtScaleEngine::Floating |
							QwtScaleEngine::IncludeReference);
  this->axisScaleEngine(QwtPlot::yLeft)->setAttributes(QwtScaleEngine::Floating |
							QwtScaleEngine::IncludeReference);


  QFont axis_font =  this->axisFont( QwtPlot::xBottom );
  axis_font.setPointSize( 7 );

  QwtText t("distance");
  t.setFont(axis_font);
  this->setAxisTitle( QwtPlot::xBottom,t);

  //-------------

  if( !title.isEmpty() ) {
    QFont title_font;
    QwtText T(title);
    
    title_font.setPointSize( 8 );
    T.setFont(title_font);
    this->setTitle( T );
    this->setWindowTitle( title );
  }
  
  std::vector<double> x_vals = df.x_values();
  std::vector<double> y_vals = df.y_values();
  angle_ = angle;

  double* x = new double[x_vals.size()];
  double* y = new double[x_vals.size()];

  int actual_size = 0;
  for( int i=0 ; i < x_vals.size() ; i++ ) {
    if( GsTL::equals( x_vals[i], df.no_data_value(), 0.001 ) ||
        GsTL::equals( y_vals[i], df.no_data_value(), 0.001 ) ) continue;

    x[actual_size] = x_vals[i];
    y[actual_size] = y_vals[i];
    pairs_.push_back( pairs[i] );
    pairs_coord_x_.push_back( x_vals[i] );
    pairs_coord_y_.push_back( y_vals[i] );

    actual_size++;
  }
      
  model1_=model;
    
  const float ymargin = 1.05;
  const float xmargin = 1.06;
  if( actual_size > 0 ) {
    float ymax = *(std::max_element( y, y+actual_size ));
    float ymin = *(std::min_element( y, y+actual_size ));
    if( ymax == 0 && ymin == 0 ) 
      this->setAxisAutoScale( QwtPlot::yLeft );
    else
      this->setAxisScale( QwtPlot::yLeft, ymin*(2-ymargin), ymax*ymargin );
  
    if( y_starts_at_zero )
      this->setAxisScale( QwtPlot::yLeft, 0.0, ymax*ymargin );

    float xmax = *(std::max_element( x, x+actual_size ));
    this->setAxisScale( QwtPlot::xBottom, 0, xmax*xmargin );  
  }

  curve1_= new QwtPlotCurve("Discrete Function");
  curve1_->setData( x, y, actual_size );

  curve2_=new QwtPlotCurve("Model");
  curve1_->attach(this);      
  curve2_->attach(this);
  
  QwtSymbol symbol;
  symbol.setStyle(QwtSymbol::XCross);
  symbol.setPen( QPen(Qt::red, 3 ) );
  symbol.setSize(7);
      
  curve1_->setSymbol(symbol);
  curve1_->setStyle(QwtPlotCurve::NoCurve);
   
  symbol.setStyle( QwtSymbol::NoSymbol );
  symbol.setPen( QPen( Qt::blue, 1 ) );
  curve2_->setSymbol(symbol);
  curve2_->setStyle(QwtPlotCurve::Lines);

  this->replot();

  max_x_ = this->axisScaleDiv( QwtPlot::xBottom )->hBound();


  /*
  QObject::connect( this, SIGNAL(plotMouseReleased(const QMouseEvent& )), 
                    this, SLOT(show_pairs_count(const QMouseEvent&)) );
  */
  delete [] x;
  delete [] y;
}


void Variog_plot::mouseReleaseEvent(QMouseEvent * event)
{
  std::cerr << "show_pairs_count() called " << std::endl;
  if( event->button() != Qt::RightButton ) return;

  const float offset=1.02;
  QFont font;
  font.setPointSize( 7 );

  if( !pairs_shown_ ) {
    for( unsigned int i=0; i < pairs_.size(); i++ ) {
      QString label;
      label.setNum( pairs_[i] );
      QwtText T(label);
      
      QwtPlotMarker * marker = new QwtPlotMarker();
      marker->attach(this);

      T.setFont(font);
      marker->setValue(pairs_coord_x_[i], pairs_coord_y_[i]*offset);
      marker->setLabel(T);
    }
    pairs_shown_ = true;
  }
  else {
    detachItems(QwtPlotItem::Rtti_PlotMarker);
    pairs_shown_ = false;
  }
  replot();
}



void Variog_plot::refresh() {
  const int discretization = 1500;

  GsTLPoint p1(0,0,0);

  //Code to decide what the increment should be in plotting the continuous model
  double incr = max_x_ / double(discretization);
  if( incr == 0 ) incr = 1;
    

  double x[discretization], y[discretization];
    
  for ( int i = 0 ; i < discretization ; i++ ) {
  	GsTLPoint p2( (p1.x()+i*incr*angle_.x()), 
	        	      (p1.y()+i*incr*angle_.y()),
		              (p1.z()+i*incr*angle_.z()) );

    GsTLVector<double> v = (p2-p1);

    x[i] = euclidean_norm( v );
    y[i] = ((*model1_)( p1, p2 ));

  }

  curve2_->setData( x, y, discretization);
  this->replot();    
}


