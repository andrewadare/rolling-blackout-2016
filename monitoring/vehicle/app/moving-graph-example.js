// Moving time-series plot using d3. Based on https://bost.ocks.org/mike/path/

define( function( require ) {
  'use strict';

  var d3 = require( 'd3' );
  var messages = require( './messages');

  d3.select( 'body' )
    .append( 'div' )
    .attr( 'class', 'graph' );

  console.log(messages.getHello());

  // Input data range
  var yMin = 0;
  var yMax = 100;

  var n = 500;        // Number of points to keep in history
  var duration = 40;  // Interval between data updates in ms
  var now = new Date( Date.now() - duration );
  var plotData = d3.range( n ).map( function() {
    return 0;
  } );

  var margin = { top: 6, right: 0, bottom: 20, left: 40 };
  var width = 960 - margin.right;
  var height = 120 - margin.top - margin.bottom;

  var x = d3.time.scale()
    .domain( [ now - ( n - 2 ) * duration, now - duration ] )
    .range( [ 0, width ] );

  var y = d3.scale.linear()
    .domain( [ yMin, yMax ] )
    .range( [ height, 0 ] );

  var line = d3.svg.line()
    .interpolate( 'basis' )
    .x( function( d, i ) {
      return x( now - ( n - 1 - i ) * duration );
    } )
    .y( function( d, i ) {
      return y( d );
    } );

  var svg = d3.select( '.graph' ).append( 'p' ).append( 'svg' )
    .attr( 'width', width + margin.left + margin.right )
    .attr( 'height', height + margin.top + margin.bottom )
    .style( 'margin-left', -margin.left + 'px' )
    .append( 'g' )
    .attr( 'transform', 'translate(' + margin.left + ',' + margin.top + ')' );

  svg.append( 'defs' ).append( 'clipPath' )
    .attr( 'id', 'clip' )
    .append( 'rect' )
    .attr( 'width', width )
    .attr( 'height', height );

  var axis = svg.append( 'g' )
    .attr( 'class', 'x axis' )
    .attr( 'transform', 'translate(0,' + height + ')' )
    .call( x.axis = d3.svg.axis().scale( x ).orient( 'bottom' ) );

  var path = svg.append( 'g' )
    .attr( 'clip-path', 'url(#clip)' )
    .append( 'path' )
    .datum( plotData )
    .attr( 'class', 'line' );

  var transition = d3.select( {} ).transition()
    .duration( duration )
    .ease( 'linear' );

  ( function tick() {
    transition = transition.each( function() {

      // update the domains
      now = new Date();
      x.domain( [ now - ( n - 2 ) * duration, now - duration ] );
      y.domain( [ 0, d3.max( plotData ) ] );

      plotData.push( y( yMin + ( yMax * ( Math.random() ) ) ) );

      // redraw the line
      svg.select( '.line' )
        .attr( 'd', line )
        .attr( 'transform', null );

      // slide the x-axis left
      axis.call( x.axis );

      // slide the line left
      path.transition()
        .attr( 'transform', 'translate(' + x( now - ( n - 1 ) * duration ) + ')' );

      // pop the old data point off the front
      plotData.shift();

    } ).transition().each( 'start', tick );
  } )();

} );

