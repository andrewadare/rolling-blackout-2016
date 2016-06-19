// Script for vehicle monitoring page

define( function( require ) {
  'use strict';

  // Modules
  var d3 = require( 'd3' );
  var Compass = require( './Compass' );
  var TiltIndicator = require( './TiltIndicator' );

  // Module-scope globals
  var nPanels = 3;
  var body = d3.select( 'body' );
  var pageWidth = body[ 0 ][ 0 ].offsetWidth;
  var panelWidth = pageWidth / nPanels;
  var panelHeight = panelWidth;

  body.append( 'h1' ).text( 'Vehicle monitoring page' );

  // Add a div to contain the svg panels
  var panelContainer = body.append( 'div' )
    .attr( 'class', 'panel-container' )
    .style( 'display', 'table' )
    .style( 'width', pageWidth + 'px' )
    .style( 'height', panelHeight + 'px' );

  // Create an array of SVG group elements - one per panel
  var i = 0;
  var panels = [];
  for ( i = 0; i < nPanels; i++ ) {
    panels.push( panelContainer
      .append( 'div' )
      .attr( 'class', 'panel-div' )
      .style( 'display', 'table-cell' )
      .style( 'vertical-align', 'middle' )
      .style( 'height', panelHeight + 'px' )
      .append( 'svg' )
      .attr( 'width', panelWidth )
      .attr( 'height', panelHeight )
      .append( 'svg:g' )
      .attr( 'transform', 'translate(1,1)' ) ); // Shift to make border visible
  }

  i = 0;
  var panelTitles = [ 'Heading', 'Roll', 'Pitch' ];
  panels.forEach( function( panel ) {

    // Add a white background rectangle
    panel.append( 'svg:rect' )
      .attr( 'width', panelWidth - 2 ) // Pad by 2px for border visibility
      .attr( 'height', panelHeight - 2 )
      .style( 'fill', 'white' )
      .style( 'stroke-width', '2px' )
      .style( 'stroke', 'black' );

    // Add panel title
    panel.append( 'svg:text' )
      .attr( 'transform', function() {
        return 'translate(' + 30 + ',' + 40 + ')';
      } )
      .text( function( d ) {
        return panelTitles[ i ];
      } );

    i++;
  } );

  var compass = new Compass();
  var tiltIndicator = new TiltIndicator();

  compass.drawHeadingNode( panels[ 0 ], panelWidth, panelHeight );
  tiltIndicator.drawTiltNode( panels[ 1 ], panelWidth, panelHeight );

  compass.randomDemo();

} );

