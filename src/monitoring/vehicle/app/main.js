// Script for vehicle monitoring page

define( function( require ) {
  'use strict';

  // Modules
  var Compass = require( './Compass' );
  var d3 = require( 'd3' );
  var IMUCalibration = require( './IMUCalibration' );
  var PIDSliders = require( './PIDSliders' );
  var SteerIndicator = require( './SteerIndicator' );
  var TiltIndicator = require( './TiltIndicator' );

  // Module-scope globals
  var nPanels = 3;
  var body = d3.select( 'body' );
  var pageWidth = body[ 0 ][ 0 ].offsetWidth;
  var panelWidth = pageWidth / nPanels;
  var row1Height = panelWidth;
  var row2Height = row1Height / 2;

  // Converts from digitized steering angle in ADC increments to degrees
  var adc2degrees = d3.scale.linear()
    .domain( [ 577, 254 ] )
    .range( [ -30, +30 ] );

  // Add a "row" div with a fixed height that spans the page
  var row1Div = body.append( 'div' )
    .attr( 'class', 'row1' )
    .style( {
      'display': 'table',
      'width': pageWidth + 'px',
      'height': row1Height + 'px'
    } );

  // Add a div for the second row of panels
  var row2Div = body.append( 'div' )
    .attr( 'class', 'row2' )
    .style( {
      'display': 'table',
      'width': pageWidth + 'px',
      'height': row2Height + 'px'
    } );

  // Add "cell" divs to rows
  for ( var i = 0; i < nPanels; i++ ) {
    row1Div.append( 'div' )
      .attr( 'class', 'row1 col' + ( i + 1 ) )
      .style( {
        'display': 'table-cell',
        'vertical-align': 'middle',
        'height': row1Height + 'px'
      } );
    row2Div.append( 'div' )
      .attr( 'class', 'row2 col' + ( i + 1 ) )
      .style( {
        'display': 'table-cell',
        'vertical-align': 'middle',
        'height': row2Height + 'px'
      } );
  }

  // body.append( 'h1' ).text( 'Vehicle monitoring page' );
  var compass = Compass();
  var rollIndicator = TiltIndicator();
  var pitchIndicator = TiltIndicator();
  var steerIndicator = SteerIndicator();

  compass
    .width( panelWidth - 2 )
    .height( row1Height - 2 )
    .title( 'Heading' );

  rollIndicator
    .width( panelWidth - 2 )
    .height( row1Height - 2 )
    .title( 'Roll' )
    .labelData( [ { label: 'Left', xf: -0.85 }, { label: 'Right', xf: +0.5 } ] );

  pitchIndicator
    .width( panelWidth - 2 )
    .height( row1Height - 2 )
    .title( 'Pitch' )
    .labelData( [ { label: 'Front', xf: -0.85 }, { label: 'Rear', xf: +0.5 } ] );

  steerIndicator
    .width( panelWidth - 2 )
    .height( row2Height - 2 )
    .title( 'Steer angle' );

  // Create initial SVG panels
  d3.select( '.row1.col1' ).datum( { heading: 0 } ).call( compass );
  d3.select( '.row1.col2' ).datum( { tilt: 0 } ).call( rollIndicator );
  d3.select( '.row1.col3' ).datum( { tilt: 10 } ).call( pitchIndicator );
  d3.select( '.row2.col1' ).datum( { angle: 0 } ).call( steerIndicator );

  PIDSliders.add( panelWidth - 2 );

  // Orientation sensor calibration status info
  d3.select( '.row2.col3' ).append( 'h2' )
    .text( 'Orientation sensor calibration status' );
  d3.select( '.row2.col3' ).append( 'ul' );

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

          // Update indicators
          d3.select( '.row1.col1' ).datum( { heading: -d.yaw * 180 / Math.PI + 90 } ).call( compass );
          d3.select( '.row1.col2' ).datum( { tilt: d.roll * 180 / Math.PI } ).call( rollIndicator );
          d3.select( '.row1.col3' ).datum( { tilt: -d.pitch * 180 / Math.PI } ).call( pitchIndicator );
          d3.select( '.row2.col1' ).datum( { angle: adc2degrees( d.sa ) } ).call( steerIndicator );
          // d3.select( '.row2.col2' ).datum( { } ).call( pidSliders );

          // Update calibration status
          var codes = IMUCalibration.unpackStatusCodes( d.AMGS );
          var data = [
            { name: 'Accel', status: codes.a },
            { name: 'Mag', status: codes.m },
            { name: 'Gyro', status: codes.g },
            { name: 'System', status: codes.s }
          ];
          IMUCalibration.updateCalibration( data );

          break;
      }
    };
  }
  // Fake, nonsensical animation loop for basic testing
  else {
    var j = 0;
    setInterval( function() {
      d3.select( '.row1.col1' ).datum( { heading: 360 * Math.sin( j / 100 ) } ).call( compass );
      d3.select( '.row1.col3' ).datum( { tilt: 10 * Math.sin( j / 5 ) } ).call( pitchIndicator );
      d3.select( '.row2.col1' ).datum( { angle: 30 * Math.cos( j / 10 ) } ).call( steerIndicator );
      IMUCalibration.updateCalibration( [
        { name: 'Accel', status: Math.floor( 3 * Math.random() ) },
        { name: 'Mag', status: Math.floor( 3 * Math.random() ) },
        { name: 'Gyro', status: Math.floor( 3 * Math.random() ) },
        { name: 'System', status: Math.floor( 3 * Math.random() ) }
      ] );

      j++;
    }, 100 );
  }

} );

