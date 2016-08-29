// Indicator representing vehicle tilt (roll or pitch)

define( function( require ) {
  'use strict';

  // Module imports
  var Arrowhead = require( './Arrowhead' );
  var d3 = require( 'd3' );

  function TiltIndicator() {

    var width = 250;
    var height = 250;

    // Margin goes between the root svg and its child <g>.
    var margin = { top: 1, right: 1, bottom: 1, left: 1 };

    var title = 'Title';

    // Labels for LHS and RHS with offset factors. Examples:
    // var leftRight = [ { label: 'Left', xf: -0.85 }, { label: 'Right', xf: +0.5 } ];
    // var frontRear = [ { label: 'Front', xf: -0.85 }, { label: 'Rear', xf: +0.5 } ];
    var labelData = [];

    // Inner "instance" function to be returned from the closure.
    // It takes a D3 selection, or equivalently selection.call(f).
    function f( selection ) {
      selection.each( function( data, i ) {

        // Outer radius of polar plot
        var r = Math.min( width, height ) / 3;

        //
        // DATA JOIN
        //

        // Select the svg element, if it exists.
        var svg = d3.select( this ).selectAll( 'svg' ).data( [ data ] );

        //
        // ENTER
        //

        // Otherwise, create the root SVG group element.
        // This <g> will be offset by the margins.
        var gEnter = svg.enter().append( 'svg' )
          .attr( 'class', 'panel' )
          .append( 'g' );

        // SVG element translated to the pivot point of the compass needle.
        var origin = gEnter.append( 'g' ).attr( 'class', 'origin' );

        // Node containing semicircle and pointer arrow
        var horizon = origin.append( 'g' )
          .attr( 'class', 'horizon' );

        // Live angle readout
        origin.append( 'svg:text' )
          .attr( 'class', 'readout' )
          .attr( 'transform', function() {
            return 'translate(' + ( -1.4 * r ) + ',' + ( -1.3 * r ) + ')';
          } )
          .text( function( d ) {
            return title + ': ' + Math.round( d.tilt ) + '°';
          } );

        // Text labels for degrees
        origin.append( 'g' )
          .attr( 'class', 'a axis' )
          .selectAll( 'g' )
          .data( d3.range( -180, +180, 30 ) )
          .enter().append( 'g' )
          .attr( 'transform', function( d ) {
            return 'rotate(' + ( d - 90 ) + ')';
          } ).append( 'text' )
          .attr( 'x', r + 6 )
          .attr( 'dy', '.35em' )
          .style( 'text-anchor', function( d ) {
            return d < 0 ? 'end' : null;
          } )
          .attr( 'transform', function( d ) {
            return d < 0 ? 'rotate(180 ' + ( r + 6 ) + ',0)' : null;
          } )
          .text( function( d ) {
            return Math.abs( d ) < 180 ? d + '°' : null;
          } );

        // Gradient to create semicircle
        var grad = horizon.append( 'defs' )
          .append( 'linearGradient' )
          .attr( {
            'id': 'grad',
            'x1': '0%',
            'x2': '0%',
            'y1': '100%',
            'y2': '0%'
          } );
        grad.append( 'stop' )
          .attr( 'offset', '50%' )
          .style( 'stop-color', '#777' );
        grad.append( 'stop' )
          .attr( 'offset', '50%' )
          .style( 'stop-color', 'white' );

        // Outermost ring
        horizon.append( 'circle' )
          .attr( 'class', 'bezel' )
          .attr( 'r', r )
          .attr( 'fill', 'url(#grad)' );

        horizon.selectAll( '.side-labels' )
          .data( labelData )
          .enter().append( 'svg:text' )
          .attr( 'class', 'side-labels' )
          .attr( 'transform', function( d ) {
            return 'translate(' + ( d.xf * r ) + ',' + ( -0.1 * r ) + ')';
          } )
          .text( function( d ) {
            return d.label;
          } );

        // SVG arrowhead
        horizon.append( Arrowhead() );

        // Pointer arrow
        horizon.append( 'line' )
          .attr( {
            'x1': 0,
            'y1': -0.6 * r,
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
            'translate(' + ( width / 2 - margin.left ) + ',' + ( height / 2 - margin.top ) + ')' );

        // Update positioning and rotation
        g.select( '.horizon' )
          .attr( 'transform', function( d ) {
            return 'rotate(' + ( d.tilt ) + ')'; //+
          } );

        // Update angle readout text
        g.select( '.readout' )
          .text( function( d ) {
            return title + ': ' + Math.round( d.tilt ) + '°';
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

    f.labelData = function( _ ) {
      if ( !arguments.length ) return labelData;
      labelData = _;
      return f;
    };

    return f;
  }

  return TiltIndicator;
} );

