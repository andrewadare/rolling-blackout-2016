// Script for vehicle monitoring page

define( function( require ) {
  'use strict';

  // Modules
  var Compass = require( './Compass' );
  var d3 = require( 'd3' );
  var IMUCalibration = require( './IMUCalibration' );
  var LidarView = require( './LidarView' );
  var SteerIndicator = require( './SteerIndicator' );
  var TiltIndicator = require( './TiltIndicator' );

  function LidarData( n ) {
    var self = this;
    this.data = Array( n ).fill( { r: 0, b: 0 } );
    this.i = 0;

    /**
     * add: add a data point to the array, wrapping around if necessary
     * @param {[Object]} datapoint - data like {r: 10, b: 45} meters, degrees
     */
    this.add = function( datapoint ) {
      if ( self.i === self.data.length ) {
        self.i = 0;
      }
      self.data[ self.i ] = datapoint;
      self.i++;
    };
  }

  // Module-scope globals
  var i = 0;
  var body = d3.select( 'body' );
  var pageWidth = body[ 0 ][ 0 ].offsetWidth;
  var pageHeight = 0.618 * pageWidth;
  var row1Height = 2 / 3 * pageHeight - 2;
  var row2Height = 1 / 3 * pageHeight - 2;
  var row1PanelWidth = pageWidth / 2 - 2;
  var row2PanelWidth = pageWidth / 4 - 2;

  // Add a div for the first row of panels
  var row1Div = body.append( 'div' )
    .attr( 'class', 'row1' )
    .style( {
      'display': 'table',
      'width': pageWidth + 'px',
      'height': row1Height + 'px'
    } );

  // second row
  var row2Div = body.append( 'div' )
    .attr( 'class', 'row2' )
    .style( {
      'display': 'table',
      'width': pageWidth + 'px',
      'height': row2Height + 'px'
    } );

  for ( i = 0; i < 2; i++ ) {
    row1Div.append( 'div' )
      .attr( 'class', 'row1 col' + ( i + 1 ) )
      .style( {
        'display': 'table-cell',
        'vertical-align': 'middle',
        'height': row1Height + 'px'
      } );
  }

  for ( i = 0; i < 4; i++ ) {
    row2Div.append( 'div' )
      .attr( 'class', 'row2 col' + ( i + 1 ) )
      .style( {
        'display': 'table-cell',
        'vertical-align': 'top',
        'height': row2Height + 'px'
      } );
  }

  // body.append( 'h1' ).text( 'Vehicle monitoring page' );

  // Make instances of objects to appear in row 1
  var lidarView = LidarView();

  // and row 2
  var compass = Compass();
  var rollIndicator = TiltIndicator();
  var pitchIndicator = TiltIndicator();
  var steerIndicator = SteerIndicator();

  lidarView
    .width( row1PanelWidth )
    .height( row1Height )
    .title( 'Lidar data' );

  compass
    .width( row2PanelWidth )
    .height( row2Height )
    .title( 'Heading' );

  rollIndicator
    .width( row2PanelWidth )
    .height( row2Height )
    .title( 'Roll' )
    .labelData( [ { label: 'Left', xf: -0.85 }, { label: 'Right', xf: +0.5 } ] );

  pitchIndicator
    .width( row2PanelWidth )
    .height( row2Height )
    .title( 'Pitch' )
    .labelData( [ { label: 'Front', xf: -0.85 }, { label: 'Rear', xf: +0.5 } ] );

  steerIndicator
    .width( row2PanelWidth )
    .height( row2Height / 2 )
    .title( 'Steer angle' );

  // Create initial SVG panels
  d3.select( '.row1.col1' ).data( [ {} ] ).call( lidarView );
  // Map here - TODO
  d3.select( '.row2.col1' ).datum( { heading: 0 } ).call( compass );
  d3.select( '.row2.col2' ).datum( { tilt: 0 } ).call( rollIndicator );
  d3.select( '.row2.col3' ).datum( { tilt: 10 } ).call( pitchIndicator );
  d3.select( '.row2.col4' ).datum( { angle: 0 } ).call( steerIndicator );

  // Orientation sensor calibration status info
  d3.select( '.row2.col4' ).append( 'h2' )
    .text( 'Orientation sensor calibration status' );
  d3.select( '.row2.col4' ).append( 'ul' );

  // var rbuf = new RingBuffer();

  if ( false ) {

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
          d3.select( '.row2.col1' ).datum( { heading: -d.yaw * 180 / Math.PI + 90 } ).call( compass );
          d3.select( '.row2.col2' ).datum( { tilt: d.roll * 180 / Math.PI } ).call( rollIndicator );
          d3.select( '.row2.col3' ).datum( { tilt: -d.pitch * 180 / Math.PI } ).call( pitchIndicator );
          d3.select( '.row2.col4' ).datum( { angle: d.sa } ).call( steerIndicator );

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
  // Fake and nonsensical animation loop for basic testing
  else {
    var j = 0;
    var fld = new LidarData( 90 );
    var angle = 356;

    setInterval( function() {

      fld.add( { r: 10 + 5 * Math.cos( j / 100 ) * Math.random(), b: angle } );
      angle = angle > 0 ? angle - 4 : 356;

      lidarView.draw( fld.data );

      d3.select( '.row2.col1' ).datum( { heading: 360 * Math.sin( j / 100 ) } ).call( compass );
      d3.select( '.row2.col3' ).datum( { tilt: 10 * Math.sin( j / 5 ) } ).call( pitchIndicator );
      d3.select( '.row2.col4' ).datum( { angle: 30 * Math.cos( j / 10 ) } ).call( steerIndicator );

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

