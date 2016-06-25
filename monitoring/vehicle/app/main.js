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

  function updateCalibration( data ) {

    function textLine( d ) {
      return d.name + ': ' + d.status;
    }

    d3.select( 'ul' ).selectAll( 'li' )
      .data( data )
      .text( textLine )
      .enter().append( 'li' )
      .attr( 'class', 'cal' )
      .text( textLine );
  }

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

  var leftRight = [ { label: 'Left', xf: -0.85 }, { label: 'Right', xf: +0.5 } ];
  var frontRear = [ { label: 'Front', xf: -0.85 }, { label: 'Rear', xf: +0.5 } ];

  compass.draw( panels[ 0 ], panelWidth, panelHeight, 'Heading' );
  rollIndicator.draw( panels[ 1 ], panelWidth, panelHeight, 'Roll', leftRight );
  pitchIndicator.draw( panels[ 2 ], panelWidth, panelHeight, 'Pitch', frontRear );

  // Orientation sensor calibration status info
  body.append( 'h2' ).text( 'Orientation sensor calibration status' );
  body.append( 'ul' );

  // From a status like 123, return { a: '0', m: '1', g: '2', s: '3' }
  function unpackStatusCodes( status ) {
    var s = status.toString();
    var codes = {};
    var keys = [ 'a', 'm', 'g', 's' ];
    var i = 0;
    while ( s.length < 4 ) {
      s = '0' + s;
    }
    keys.forEach( function( k ) {
      codes[ k ] = s.charAt( i );
      i++;
    } );
    return codes;
  }

  if ( true ) {

    // Initiate a WebSocket connection
    var ws = new WebSocket( 'ws://' + window.location.host );
    ws.onopen = function( event ) {
      ws.send( JSON.stringify( {
        type: 'update',
        text: 'ready',
        id: 0,
        date: Date.now()
      } ) );
    };

    // Handler for messages received from server
    ws.onmessage = function( event ) {
      var msg = JSON.parse( event.data );
      var d = msg.data;
      switch ( msg.type ) {
        case 'quaternions':
          // console.log( d );
          compass.update( -d.yaw * 180 / Math.PI + 90 );
          pitchIndicator.update( -d.pitch * 180 / Math.PI );
          rollIndicator.update( d.roll * 180 / Math.PI );

          // Update calibration status list with fake calibration data
          var codes = unpackStatusCodes( d.AMGS );
          var data = [
            { name: 'Accel', status: codes.a },
            { name: 'Mag', status: codes.m },
            { name: 'Gyro', status: codes.g },
            { name: 'System', status: codes.s }
          ];
          updateCalibration( data );
          break;
        case 'steering':
          console.log( d.adc );
          break;
      }
    };
  }


  // Test function - update the page continuously with fake random data
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

        if ( steps % 100 === 0 ) {

          // Update calibration status list with fake calibration data
          var data = [
            { name: 'Accel', status: Math.round( 3 * Math.random() ) },
            { name: 'Mag', status: Math.round( 3 * Math.random() ) },
            { name: 'Gyro', status: Math.round( 3 * Math.random() ) },
            { name: 'System', status: Math.round( 3 * Math.random() ) }
          ];
          updateCalibration( data );
        }

        steps++;
        loop();
      }, loopDelay );
    }
    loop();
  };

  // randomDemo();

} );

