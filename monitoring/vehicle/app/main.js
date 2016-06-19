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
  var rollIndicator = new TiltIndicator();
  var pitchIndicator = new TiltIndicator();

  compass.draw( panels[ 0 ], panelWidth, panelHeight );
  rollIndicator.draw( panels[ 1 ], panelWidth, panelHeight );
  pitchIndicator.draw( panels[ 2 ], panelWidth, panelHeight );

  // Test function - make random direction changes
  var randomDemo = function() {
    var heading = 0; // deg
    var roll = 0;
    var pitch = 0;
    var steps = 0;
    var sign = 0;
    var loopDelay = 1 / 30; // ms
    function loop() {
      setTimeout( function() {
        compass.update( heading );


        // Change direction every once in a while
        if ( steps % Math.round( 400 * Math.random() + 100 ) === 0 ) {
          sign = Math.round( Math.random() ) ? +1 : -1;
        }
        heading += sign * 0.2 * Math.random();
        roll += 0.5 * (Math.random() - 0.5);
        rollIndicator.update( roll );
        pitch += 0.5 * (Math.random() - 0.5);
        pitchIndicator.update( pitch );

        if (Math.abs(roll) > 10) roll /= 2;
        if (Math.abs(pitch) > 15) pitch /= 2;

        steps++;
        loop();
      }, loopDelay );
    }
    loop();
  };

  randomDemo();

} );

