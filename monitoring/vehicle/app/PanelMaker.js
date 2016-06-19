// Plain-Jane background panel generator for dashboard.
// Currently not very configurable; change as needed.

define( function( require ) {
  'use strict';

  function PanelMaker( panelWidth, panelHeight ) {

    this.width = panelWidth;
    this.height = panelHeight;

    this.addTo = function( containerDiv ) {

      var self = this;

      var panel = containerDiv
        .append( 'div' )
        .attr( 'class', 'panel-div' )
        .style( 'display', 'table-cell' )
        .style( 'vertical-align', 'middle' )
        .style( 'height', self.height + 'px' )
        .append( 'svg' )
        .attr( 'width', self.width )
        .attr( 'height', self.height )
        .append( 'svg:g' )
        .attr( 'transform', 'translate(1,1)' ); // Shift to make border visible

      // Add a white background rectangle
      panel.append( 'svg:rect' )
        .attr( 'width', self.width - 2 ) // Pad by 2px for border visibility
        .attr( 'height', self.height - 2 )
        .style( 'fill', 'white' )
        .style( 'stroke-width', '2px' )
        .style( 'stroke', 'black' );

      return panel;
    };
  }

  return PanelMaker;
} );

