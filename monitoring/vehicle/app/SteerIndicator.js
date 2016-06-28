// Steering angle indicator

define( function( require ) {
  'use strict';

  // Module imports
  var d3 = require( 'd3' );
  var Arrowhead = require( './Arrowhead' );

  function SteerIndicator() {
    var width = 250;
    var height = 250;

    // Margin goes between the root svg and its child <g>.
    var margin = { top: 1, right: 1, bottom: 1, left: 1 };

    var title = 'Title';

    // Inner "instance" function to be returned from the closure.
    // It takes a D3 selection, or equivalently selection.call(f).
    function f( selection ) {
      selection.each( function( data, i ) {

        // Outer radius of polar plot
        var r = 0.75 * width;

        //
        // DATA JOIN
        //

        // Select the svg element, if it exists.
        var svg = d3.select( this ).selectAll( 'svg' ).data( [ data ] );

        //
        // ENTER
        //

        // Otherwise, create the root SVG group element.
        // This <g> is will be offset by the margins.
        var gEnter = svg.enter().append( 'svg' )
          .style( 'background-color', 'white' )
          .append( 'g' );

        // SVG element translated to the pivot point of the compass needle.
        var origin = gEnter.append( 'g' )
          .attr( 'class', 'origin' );
        // .attr( 'transform', 'translate(' + width / 2 + ',' + 1.8 * height + ')' );

        // Text labels for degrees
        origin.append( 'g' )
          .attr( 'class', 'a axis' )
          .selectAll( 'g' )
          .data( d3.range( -30, +35, 5 ) )
          .enter().append( 'g' )
          .attr( 'transform', function( d ) {
            return 'rotate(' + ( d - 90 ) + ')';
          } ).append( 'text' )
          .attr( 'x', r + 10 )
          .attr( 'dy', '.35em' )
          .style( 'text-anchor', function( d ) {
            return 'end';
          } )
          .text( function( d ) {
            return Math.abs( d ) < 180 ? d + '°' : null;
          } );

        // Live angle readout
        origin.append( 'svg:text' )
          .attr( 'class', 'readout' )
          .attr( 'transform', function() {
            return 'translate(' + ( -0.6 * r ) + ',' + ( -1.1 * r ) + ')';
          } )
          .text( function() {
            return title + ': ' + '0°';
          } )
          .attr( 'font-size', '20px' );

        // Node containing pointer arrow
        var pointer = origin.append( 'g' )
          .attr( 'class', 'pointer' );

        pointer.append( Arrowhead() );

        // Pointer arrow line
        pointer.append( 'line' )
          .style( 'stroke', 'black' )
          .style( 'stroke-width', '4px' )
          .attr( {
            'x1': 0,
            'y1': -0.75 * r,
            'x2': 0,
            'y2': -0.9 * r,
            'marker-end': 'url(#arrow)',
            'class': 'needle'
          } );

        //
        // UPDATE
        //

        // Update the outer dimensions
        svg.attr( 'width', width )
          .attr( 'height', height );

        // Update the inner dimensions
        var g = svg.select( 'g' )
          .attr( 'transform', 'translate(' + margin.left + ',' + margin.top + ')' );

        // Update the needle origin
        g.select( '.origin' )
          .attr( 'transform',
            'translate(' + ( width / 2 - margin.left ) + ',' + ( 1.8 * height - margin.top ) + ')' );

        // Update pointer angle
        g.select( '.pointer' )
          .attr( 'transform', function( d ) {
            return 'rotate(' + ( d.angle ) + ')';
          } );

        // Update angle readout text
        g.select( '.readout' )
          .text( function( d ) {
            return title + ': ' + Math.round( d.angle ) + '°';
          } );

      } );
    }

    // Chainable getter/setter methods

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
  return SteerIndicator;
} );

