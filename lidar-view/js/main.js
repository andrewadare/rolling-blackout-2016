( function() {
  'use strict';

  var width = 960;
  var height = 960;
  var center = {
    x: width / 2,
    y: height / 2
  };
  var pi = Math.PI;
  var dataMax = 600; // Max view range of sensor data [cm]

  var svg = d3.select( 'body' ).append( 'svg' )
    .attr( 'width', width )
    .attr( 'height', height );

  // Map data to SVG dimensions
  var padFactor = 0.1;
  var x = d3.scale.linear()
    .domain( [ 0, 2 * dataMax ] )
    .range( [ padFactor * width, ( 1 - padFactor ) * width ] );
  var y = d3.scale.linear()
    .domain( [ 0, 2 * dataMax ] )
    .range( [ ( 1 - padFactor ) * height, padFactor * height ] );

  // Array of recent datapoints
  var sensorData = [];

  // Initiate a WebSocket connection
  var ws = new WebSocket( 'ws://' + window.location.host );

  function drawAxes() {

    svg.append( 'line' )
      .attr( 'x1', 0 )
      .attr( 'y1', height / 2 )
      .attr( 'x2', width )
      .attr( 'y2', height / 2 )
      .attr( 'class', 'axes' );

    svg.append( 'line' )
      .attr( 'x1', width / 2 )
      .attr( 'y1', 0 )
      .attr( 'x2', width / 2 )
      .attr( 'y2', height )
      .attr( 'class', 'axes' );

    var rings = [ 1, 100, 200, 300, 400, 500, 600 ];

    rings.forEach( function( r ) {
      svg.append( 'circle' )
        .attr( 'class', 'axes' )
        .attr( 'cx', width / 2 )
        .attr( 'cy', height / 2 )
        .attr( 'r', r * width / ( 2 * dataMax ) );
    } );

  }

  function drawData( data ) {
    var getx = function( d ) {
      return x( dataMax + d.rcm * Math.cos( d.phi ) );
    }

    var gety = function( d ) {
      return y( dataMax + d.rcm * Math.sin( d.phi ) );
    }

    svg.selectAll( '.data' )
      .data( data )
      .attr( 'cx', getx )
      .attr( 'cy', gety )
      .attr( 'r', 4 )
      .enter().append( 'circle' )
      .attr( 'class', 'data' )
      .attr( 'r', 4 )
      .attr( 'cx', getx )
      .attr( 'cy', gety );

    // Append these lines to fade the points out
    // .transition()
    // .duration( 1000 )
    // .style( 'opacity', 0 )
    // .each( 'end', function() {
    //   d3.select( this )
    //     .remove();
    // } );

  }

  ws.onopen = function( event ) {
    ws.send( JSON.stringify( {
      type: 'update',
      text: 'ready',
      id: 0,
      date: Date.now()
    } ) );
  }

  // Handler for messages received from server
  ws.onmessage = function( event ) {
    var msg = JSON.parse( event.data );
    switch ( msg.type ) {
      case 'sensor_data':
        sensorData.push( msg.data );
        drawData( sensorData );
        if ( sensorData.length > 20 ) {
          sensorData.shift();
        }
        break;
      case 'fake_data':
        sensorData.push( msg.data );
        drawData( sensorData );
        if ( sensorData.length > 500 ) {
          sensorData.shift();
        }
        break;

        //
        // Handle more messages here as needed
        //
    }
  }

  drawAxes();
} )();

