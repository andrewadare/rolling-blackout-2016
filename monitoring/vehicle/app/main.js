// Script for vehicle monitoring page

define( function( require ) {
  'use strict';

  // Modules
  var d3 = require( 'd3' );
  var Compass = require( './Compass' );
  var PanelMaker = require( './PanelMaker' );
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
    .style( {
      'display': 'table',
      'width': pageWidth + 'px',
      'height': panelHeight + 'px'
    } );

  // Create an array of background panels
  var panels = [];
  var panelMaker = new PanelMaker( panelWidth, panelHeight );
  for ( var i = 0; i < nPanels; i++ ) {
    panels.push( panelMaker.addTo( panelContainer ) );
  }

  var compass = new Compass();
  var rollIndicator = new TiltIndicator();
  var pitchIndicator = new TiltIndicator();

  compass.draw( panels[ 0 ], panelWidth, panelHeight, 'Heading' );
  rollIndicator.draw( panels[ 1 ], panelWidth, panelHeight, 'Roll' );
  pitchIndicator.draw( panels[ 2 ], panelWidth, panelHeight, 'Pitch' );

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

        roll += 0.5 * ( Math.random() - 0.5 );
        rollIndicator.update( roll );

        pitch += 0.5 * ( Math.random() - 0.5 );
        pitchIndicator.update( pitch );

        ( Math.abs( roll ) > 10 ) && ( roll /= 2 );
        ( Math.abs( pitch ) > 15 ) && ( pitch /= 2 );

        steps++;
        loop();
      }, loopDelay );
    }
    loop();
  };

  randomDemo();

} );

