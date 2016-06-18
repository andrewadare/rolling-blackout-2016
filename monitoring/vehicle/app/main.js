// Script for vehicle monitoring page

define( function( require ) {
  'use strict';

  // Modules
  var d3 = require( 'd3' );

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

  // To center something, do something like .attr( 'transform', toCenter )
  var toCenter = function() {
    return 'translate(' + panelWidth / 2 + ',' + panelHeight / 2 + ')';
  };

  i = 0;
  var panelTitles = [ 'Heading', 'Roll', 'Pitch' ];
  panels.forEach( function( panel ) {

    // Add a white background rectangle
    panel.append( 'svg:rect' )
      .attr( 'width', 0.99 * panelWidth )
      .attr( 'height', 0.99 * panelHeight )
      .style( 'fill', 'white' )
      .style( 'stroke-width', '1px' )
      .style( 'stroke', 'black' );

    // Add panel title
    panel.append( 'svg:text' )
      .attr( 'transform', function() {
        return 'translate(' + 0.1 * panelWidth + ',' + 0.1 * panelHeight + ')';
      } )
      .text( function( d ) {
        return panelTitles[ i ];
      } );

    // Add a circle
    panel.append( 'svg:circle' )
      .attr( 'transform', toCenter )
      .attr( 'r', panelWidth / 3 )
      .style( 'fill', 'none' )
      .style( 'stroke', 'blue' );

    i++;
  } );

} );

