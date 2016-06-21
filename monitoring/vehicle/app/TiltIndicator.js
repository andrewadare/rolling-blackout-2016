// Node in scene graph representing vehicle tilt

define( function( require ) {
  'use strict';

  // Module imports
  var d3 = require( 'd3' );

  // Constructor
  function TiltIndicator() {

    this.tilt = 0; // Angle in degrees from vertical

    // Root SVG node to draw in
    this.rootNode = null;

    this.title = 'Title';

    this.update = function( tilt ) {

      this.tilt = tilt;

      var self = this;

      // Update node with semicircle and pointer
      this.rootNode.select( '.horizon' )
        .attr( 'transform', function() {
          return 'rotate(' + ( tilt ) + ')';
        } );

      // Update angle readout text
      this.rootNode.select( '.readout' )
        .text( function() {
          return self.title + ': ' + Math.round( tilt ) + '°';
        } );

    };

    this.draw = function( parentSVG, width, height, title, labelData ) {

      this.title = title;

      var self = this;

      // Root SVG element to add as a child to the provided parentSVG
      this.rootNode = parentSVG.append( 'g' )
        .attr( 'transform', 'translate(' + width / 2 + ',' + height / 2 + ')' );

      // Outer radius of polar plot
      var r = Math.min( width, height ) / 3;

      // Text labels for degrees
      this.rootNode.append( 'g' )
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

      // Live angle readout
      this.rootNode.append( 'svg:text' )
        .attr( 'class', 'readout' )
        .attr( 'transform', function() {
          return 'translate(' + ( -1.4 * r ) + ',' + ( -1.3 * r ) + ')';
        } )
        .text( function() {
          return Math.round( self.tilt ) + '°';
        } )
        .attr( 'font-size', '20px' );

      // Node containing semicircle and pointer arrow
      var horizon = this.rootNode.append( 'g' )
        .attr( 'class', 'horizon' );

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
        .attr( 'r', r )
        .attr( 'stroke', '#777' )
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
        } )
        .attr( 'font-size', '16px' );


      // SVG arrowhead
      horizon.append( 'defs' ).append( 'marker' )
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
      horizon.append( 'line' )
        .style( 'stroke', 'black' )
        .style( 'stroke-width', '4px' )
        .attr( {
          'x1': 0,
          'y1': -0.6 * r,
          'x2': 0,
          'y2': -0.9 * r,
          'marker-end': 'url(#arrow)',
          'class': 'needle'
        } );

    };
  }

  return TiltIndicator;
} );

