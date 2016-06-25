// Node in scene graph representing Steering angle

define( function( require ) {
  'use strict';

  // Module imports
  var d3 = require( 'd3' );

  // Constructor
  function SteerIndicator() {

    // Full left(right) = -30(+30) degrees.
    this.steerAngle = 0;

    // Root SVG node to draw in
    this.rootNode = null;

    // TODO
    this.update = function( angle ) {
      this.steerAngle = angle;

      var self = this;

      // Update node with semicircle and pointer
      this.rootNode.select( '.pointer' )
        .attr( 'transform', function() {
          return 'rotate(' + ( angle ) + ')';
        } );

      // Update angle readout text
      this.rootNode.select( '.readout' )
        .text( function() {
          return self.title + ': ' + Math.round( angle ) + '°';
        } );

    };

    this.draw = function( parentSVG, width, height, title ) {

      this.title = title;

      // Root SVG element to add as a child to the provided parentSVG
      this.rootNode = parentSVG.append( 'g' )
        .attr( 'transform', 'translate(' + width / 2 + ',' + 1.8 * height + ')' );

      // Outer radius of polar plot
      var r = 0.75 * width;

      // Text labels for degrees
      this.rootNode.append( 'g' )
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
      this.rootNode.append( 'svg:text' )
        .attr( 'class', 'readout' )
        .attr( 'transform', function() {
          return 'translate(' + ( -0.6 * r ) + ',' + ( -1.1 * r ) + ')';
        } )
        .text( function() {
          return title + ': ' + '0°';
        } )
        .attr( 'font-size', '20px' );

      // Node containing pointer arrow
      var pointer = this.rootNode.append( 'g' )
        .attr( 'class', 'pointer' );

      // SVG arrowhead
      pointer.append( 'defs' ).append( 'marker' )
        .attr( {
          'id': 'arrow',
          'viewBox': '0 -5 10 10',
          'refX': 5,
          'refY': 0,
          'markerWidth': 4,
          'markerHeight': 4,
          'orient': 'auto'
        } )
        .append( 'path' )
        .attr( 'd', 'M0,-5L10,0L0,5' );

      // Pointer arrow
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

    };
  }
  return SteerIndicator;
} );

