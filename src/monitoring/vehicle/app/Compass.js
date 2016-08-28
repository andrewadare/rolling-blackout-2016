// Compass

define( function( require ) {
  'use strict';

  // Module imports
  var d3 = require( 'd3' );
  var Arrowhead = require( './Arrowhead' );

  function Compass() {
    var width = 250;
    var height = 250;
    var title = 'Title';

    // Margin goes between the root svg and its child <g>.
    var margin = { top: 1, right: 1, bottom: 1, left: 1 };

    // Length of compass needle
    var needleRadius = Math.min( width, height ) / 3;

    // x and y position of compass needle tip
    var rx = function( d ) {
      var phi = d.heading * Math.PI / 180 - Math.PI / 2;
      return 0.9 * needleRadius * Math.cos( phi );
    };
    var ry = function( d ) {
      var phi = d.heading * Math.PI / 180 - Math.PI / 2;
      return 0.9 * needleRadius * Math.sin( phi );
    };

    // Inner "instance" function to be returned from the closure.
    // It takes a D3 selection, or equivalently selection.call(f).
    function f( selection ) {
      selection.each( function( data, i ) {

        //
        // DATA JOIN
        //

        // Select the svg element, if it exists.
        var svg = d3.select( this ).selectAll( 'svg' ).data( [ data ] );

        //
        // ENTER
        //

        // Otherwise, create the root SVG group element.
        var gEnter = svg.enter().append( 'svg' )
          .attr( 'class', 'panel' )
          .append( 'g' );

        // SVG element translated to the pivot point of the compass needle.
        var origin = gEnter.append( 'g' ).attr( 'class', 'origin' );

        // Outer radius of polar plot
        needleRadius = Math.min( width, height ) / 3;

        // Linear radial scale function
        var r = d3.scale.linear()
          .domain( [ 0, .5 ] )
          .range( [ 0, needleRadius ] );

        // Radial ring(s) - not really necessary here, using only outermost ring.
        // Keeping for possible future use as a speed scale (arrow length could be
        // used as a speed indicator)
        origin.append( 'g' )
          .attr( 'class', 'bezel compass' )
          .selectAll( 'g' )
          .data( r.ticks( 1 ).slice( 1 ) ) // Increase r.ticks for more rings
          .enter().append( 'g' )
          .append( 'circle' )
          .attr( 'r', r );

        // Text labels for degrees
        origin.append( 'g' )
          .attr( 'class', 'a axis' )
          .selectAll( 'g' )
          .data( d3.range( 0, 360, 30 ) )
          .enter().append( 'g' )
          .attr( 'transform', function( d ) {
            return 'rotate(' + ( d - 90 ) + ')';
          } ).append( 'text' )
          .attr( 'x', needleRadius + 6 )
          .attr( 'dy', '.35em' )
          .style( 'text-anchor', function( d ) {
            return d > 180 ? 'end' : null;
          } )
          .attr( 'transform', function( d ) {
            return d > 180 ? 'rotate(180 ' + ( needleRadius + 6 ) + ',0)' : null;
          } )
          .text( function( d ) {
            return d + '°';
          } );

        // SVG arrowhead
        origin.append( Arrowhead() );

        // Compass arrow
        origin.append( 'line' )
          .attr( {
            'x1': 0,
            'y1': 0,
            'x2': rx,
            'y2': ry,
            'marker-end': 'url(#arrow)',
            'class': 'needle'
          } );

        // Live angle readout
        origin.append( 'svg:text' )
          .attr( 'class', 'readout' )
          .attr( 'transform', function() {
            return 'translate(' + ( -1.4 * needleRadius ) + ',' + ( -1.3 * needleRadius ) + ')';
          } )
          .text( function() {
            return title + ': ' + '0°';
          } );

        //
        // UPDATE
        //

        // Update compass angle from data
        svg.select( '.needle' ).attr( {
          'x2': rx,
          'y2': ry
        } );

        // Update the angle readout text from data
        svg.select( '.readout' )
          .attr( 'transform', function() {
            return 'translate(' + ( -1.4 * needleRadius ) + ',' + ( -1.3 * needleRadius ) + ')';
          } )
          .text( function( d ) {
            return title + ': ' + Math.round( d.heading ) + '°';
          } );

        // Update the outer dimensions
        svg.attr( 'width', width )
          .attr( 'height', height );

        // Update the inner dimensions
        var g = svg.select( 'g' )
          .attr( 'transform', 'translate(' + margin.left + ',' + margin.top + ')' );

        // Update the needle origin
        g.select( '.origin' )
          .attr( 'transform',
            'translate(' + ( width / 2 - margin.left ) + ',' + ( height / 2 - margin.top ) + ')' );

      } );
    }

    // To add configurability to this object, add chainable getter/setter methods.

    f.width = function( _ ) {
      if ( !arguments.length ) return width;
      width = _;
      return f;
    };

    f.height = function( _ ) {
      if ( !arguments.length ) return height;
      height = _;
      return f;
    };

    f.margin = function( _ ) {
      if ( !arguments.length ) return margin;
      margin = _;
      return f;
    };

    f.title = function( _ ) {
      if ( !arguments.length ) return title;
      title = _;
      return f;
    };

    return f;
  }

  return Compass;
} );

