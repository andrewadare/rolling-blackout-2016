// Lidar range and bearing data visualization

define( function( require ) {
  'use strict';

  // Module imports
  var d3 = require( 'd3' );

  /**
   * Class to display data points from rotating lidar sensor
   *
   * @param {number} nPoints - fixed number of continually-updated data points to draw
   */
  function LidarView( nPoints ) {
    // Default dimensions
    var width = 250;
    var height = 250;

    var pi = Math.PI;

    // Outer radius of polar plot
    var outerRadius = Math.min( width, height ) / 2;

    // Initialize linear radial scale
    var rScale = d3.scale.linear()
      .domain( [ 0, 40 ] )
      .range( [ 0, outerRadius ] );

    // Margin goes between the root svg and its child <g>.
    var margin = { top: 1, right: 1, bottom: 1, left: 1 };

    var title = 'Title';

    var origin = null;
    var rAxis = d3.svg.axis();

    // Inner "instance" function to be returned from the closure.
    // It takes a D3 selection, or equivalently selection.call(f).
    function f( selection ) {
      selection.each( function( data, i ) {

        outerRadius = Math.min( width, height ) / 2;

        // Axis SVG object for tick labels
        rAxis = d3.svg.axis()
          .scale( rScale )
          .ticks( 5 );

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

        // SVG element translated to the center of the display
        origin = gEnter.append( 'g' ).attr( 'class', 'origin' );

        // Radial rings for scale
        origin.append( 'g' )
          .attr( 'class', 'lidar-scale' )
          .selectAll( 'g' )
          .data( rScale.ticks( 5 ) )
          .enter().append( 'g' )
          .append( 'circle' )
          .attr( 'r', rScale );

        // Crosshairs
        origin.append( 'line' )
          .attr( {
            'x1': -outerRadius,
            'y1': 0,
            'x2': +outerRadius,
            'y2': 0,
            'class': 'lidar-scale'
          } );
        origin.append( 'line' )
          .attr( {
            'x1': 0,
            'y1': -outerRadius,
            'x2': 0,
            'y2': +outerRadius,
            'class': 'lidar-scale'
          } );

        origin.append( 'g' )
          .attr( 'class', 'lidar-scale' )
          .call( rAxis );

        // Update the outer dimensions
        svg.attr( 'width', width )
          .attr( 'height', height );

        // Update the inner dimensions
        var g = svg.select( 'g' )
          .attr( 'transform', 'translate(' + margin.left + ',' + margin.top + ')' );

        // Update plot origin
        g.select( '.origin' )
          .attr( 'transform',
            'translate(' + ( width / 2 - margin.left ) + ',' + ( height / 2 - margin.top ) + ')' );

      } );
    }

    // Functions to compute (x,y) of one datapoint like {r: 10, b: 270}
    function x( d ) {
      return rScale( d.r * Math.cos( pi / 180 * d.b ) );
    }

    function y( d ) {
      return -rScale( d.r * Math.sin( pi / 180 * d.b ) );
    }

    f.draw = function( data ) {

      // Dynamically rescale the radial axis
      rScale
        .domain( [ 0, 1.1 * d3.max( data, function( d ) {
          return d.r;
        } ) ] )
        .range( [ 0, outerRadius ] );

      origin.selectAll( '.lidar-scale' )
        .call( rAxis );

      origin.selectAll( '.lidar-scale circle' )
        .data( rScale.ticks( 5 ) )
        .attr( 'r', rScale );

      origin.selectAll( '.lidar-points' )
        .data( data )
        .attr( { 'cx': x, 'cy': y } )
        .attr( 'class', 'lidar-points' )
        .enter().append( 'circle' )
        .attr( 'r', 0 )
        .transition()
        .duration( 750 )
        .attr( 'r', 4 )
        .attr( { 'cx': x, 'cy': y } )
        .attr( 'class', 'lidar-points' );
    };


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

  return LidarView;
} );

