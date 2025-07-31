/**
 * @file DMC Interactive Visualizer
 * @brief Web-based visualization for DMC clustering results
 * @author DMC Team
 * @version 1.0
 */

/**
 * @class DMCVisualizer
 * @brief Interactive visualization class for DMC clustering analysis
 * 
 * This class provides a comprehensive visualization interface for DMC clustering results,
 * including hierarchical cluster display, interactive node exploration, and filtering capabilities.
 */
class DMCVisualizer {
    /**
     * @brief Constructor for DMCVisualizer
     * 
     * Initializes the visualizer with default settings, sets up data structures,
     * configures DOM caches for performance, and starts the data loading process.
     */
    constructor() {
        // Core data structures
        this.nodes = [];
        this.edges = [];
        this.clusters = [];
        
        // Display configuration
        this.width = window.innerWidth - 300;
        this.height = window.innerHeight - 80;
        this.showClusters = true;
        this.currentFilter = 'all';
        this.selectedNode = null;
        
        // D3.js visualization components
        this.simulation = null;
        this.svg = null;
        this.g = null;
        this.zoom = null;
        this.nodeElements = null;
        this.edgeElements = null;
        
        // Performance optimizations - cache frequently accessed DOM elements
        this.domCache = {
            nodeDetails: null,
            tooltip: null,
            clusterList: null,
            totalNodes: null,
            totalEdges: null,
            totalClusters: null,
            visibleNodes: null,
            loading: null
        };
        
        // Cache for expensive computations
        this.computationCache = {
            nodesByParent: new Map(),
            edgesByNode: new Map(),
            clusterChildren: new Map()
        };
        
        // Performance tracking variables
        this.currentHighlightedNode = null;
        this.hasActiveHighlights = false;
        this.currentTooltipNode = null;
        
        this.setupEventListeners();
        this.loadData();
    }
    
    /**
     * @brief Initialize the visualizer
     * @deprecated This method is redundant as initialization is handled in constructor
     */
    init() {
        this.setupSVG();
        this.setupEventListeners();
        this.loadData();
    }
    
    /**
     * @brief Set up the SVG canvas and zoom behavior
     * 
     * Creates the main SVG element, configures zoom/pan functionality,
     * adds background for interaction, and creates the main drawing group.
     */
    setupSVG() {
        this.svg = d3.select('#graph')
            .attr('width', this.width)
            .attr('height', this.height);
            
        // Add zoom behavior and store reference for programmatic control
        this.zoom = d3.zoom()
            .scaleExtent([0.1, 4])
            .on('zoom', (event) => {
                this.g.attr('transform', event.transform);
            });
            
        this.svg.call(this.zoom);
        
        // Add background for zoom/pan BEFORE creating the main group
        this.svg.append('rect')
            .attr('width', this.width)
            .attr('height', this.height)
            .attr('fill', 'transparent')
            .style('cursor', 'move')
            .style('pointer-events', 'all')
            .on('click', () => {
                // Clear selection when clicking on background
                this.clearSelection();
            });
        
        // Create main group for all elements AFTER the background
        this.g = this.svg.append('g');
    }
    
    /**
     * @brief Set up event listeners for user interface controls
     * 
     * Configures event handlers for filter controls, cluster toggles, view controls,
     * and window resize events. Uses throttled resize handler for performance.
     */
    setupEventListeners() {
        // Cache DOM elements during setup
        this.cacheDOMElements();
        
        // Use throttled resize handler
        window.addEventListener('resize', this.throttle(() => this.handleResize(), 100));
        
        // Filter controls - handle both buttons and select dropdown
        const filterButtons = document.querySelectorAll('.filter-control button');
        filterButtons.forEach(button => {
            button.addEventListener('click', (e) => {
                this.filterClusters(e.target.dataset.filter);
            });
        });
        
        // Handle the cluster filter dropdown
        const clusterFilter = document.getElementById('cluster-filter');
        if (clusterFilter) {
            clusterFilter.addEventListener('change', (e) => {
                this.filterClusters(e.target.value);
            });
        }
        
        document.getElementById('center-view').addEventListener('click', () => {
            this.centerView();
        });
    }
    
    
    /**
     * @brief Cache frequently accessed DOM elements for performance optimization
     * 
     * Stores references to commonly used DOM elements to avoid repeated queries,
     * improving overall performance during frequent updates.
     */
    cacheDOMElements() {
        this.domCache.nodeDetails = document.getElementById('node-details');
        this.domCache.tooltip = document.getElementById('tooltip');
        this.domCache.clusterList = document.getElementById('cluster-list');
        this.domCache.totalNodes = document.getElementById('total-nodes');
        this.domCache.totalEdges = document.getElementById('total-edges');
        this.domCache.totalClusters = document.getElementById('total-clusters');
        this.domCache.visibleNodes = document.getElementById('visible-nodes');
        this.domCache.loading = document.getElementById('loading');
    }

    /**
     * @brief Throttle function to limit event frequency
     * @param {Function} func - The function to throttle
     * @param {number} delay - The minimum delay between function calls in milliseconds
     * @return {Function} The throttled function
     * 
     * Creates a throttled version of a function that limits how often it can be called,
     * useful for performance optimization of frequent events like resize or scroll.
     */
    throttle(func, delay) {
        let timeoutId;
        let lastExecTime = 0;
        return function (...args) {
            const currentTime = Date.now();
            
            if (currentTime - lastExecTime > delay) {
                func.apply(this, args);
                lastExecTime = currentTime;
            } else {
                clearTimeout(timeoutId);
                timeoutId = setTimeout(() => {
                    func.apply(this, args);
                    lastExecTime = Date.now();
                }, delay - (currentTime - lastExecTime));
            }
        };
    }
    
    /**
     * @brief Asynchronously load and process visualization data
     * 
     * Fetches cluster data from data.json, validates the response, processes
     * the hierarchical data structure, and initializes the visualization.
     * Includes timeout protection and comprehensive error handling.
     */
    async loadData() {
        this.showLoading(true);
        
        // Add a timeout to prevent infinite loading
        const timeoutId = setTimeout(() => {
            console.error('Data loading timed out after 10 seconds');
            this.showError('Data loading timed out. Please check the console for details.');
            this.showLoading(false);
        }, 10000);
        
        try {
            // Check if we're running from file:// protocol
            if (window.location.protocol === 'file:') {
                console.warn('WARNING: Running from file:// protocol. CORS may prevent data loading.');
            }
            
            const response = await fetch('data.json');
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            const rawData = await response.json();
            
            if (!rawData || !Array.isArray(rawData) || rawData.length === 0) {
                throw new Error('Data is empty or not in expected format');
            }
            
            this.processHierarchicalData(rawData);
            
            if (this.nodes.length === 0) {
                throw new Error('No nodes were generated from the data');
            }
            
            this.buildPerformanceCaches();
            this.setupSVG();
            this.createVisualization();
            this.updateStatistics();
            this.updateClusterList();
            
            clearTimeout(timeoutId);
            
        } catch (error) {
            clearTimeout(timeoutId);
            console.error('Error loading data:', error);
            this.showError(`Failed to load data: ${error.message}`);
        }
        
        this.showLoading(false);
    }
    
    /**
     * @brief Process hierarchical cluster data into nodes and edges
     * @param {Array} clusters - Array of cluster objects to process
     * 
     * Converts the raw cluster data into a graph structure with nodes and edges,
     * handling cluster hierarchies, definitions, and their relationships.
     */
    processHierarchicalData(clusters) {
        
        this.nodes = [];
        this.edges = [];
        this.clusters = [];
        
        // Initialize global counters for each cluster type
        this.clusterTypeCounters = {};
        
        // Initialize a global cluster ID counter to ensure uniqueness
        this.globalClusterIdCounter = 0;
        
        try {
            this.processClusterHierarchy(clusters, null, 0);
            this.processPendingConnections();
        } catch (error) {
            console.error('Error in processHierarchicalData:', error);
            throw error;
        }
    }
    
    /**
     * @brief Recursively process cluster hierarchy to create nodes and edges
     * @param {Array} clusters - Array of cluster objects to process
     * @param {string|null} parentCluster - ID of the parent cluster, null for root level
     * @param {number} depth - Current depth in the hierarchy
     * 
     * Processes clusters at each level of the hierarchy, creating cluster nodes,
     * definition nodes, and their interconnecting edges.
     */
    processClusterHierarchy(clusters, parentCluster, depth) {
        
        if (!Array.isArray(clusters)) {
            console.error('Clusters is not an array:', clusters);
            return;
        }
        
        clusters.forEach((cluster, index) => {
            // Create a globally unique cluster ID 
            const clusterId = `cluster_${this.globalClusterIdCounter}`;
            this.globalClusterIdCounter++;
            
            // Create a unique label using global counter for each cluster type
            const clusterType = (cluster.type || 'unknown').toUpperCase();
            
            // Initialize counter for this cluster type if it doesn't exist
            if (!this.clusterTypeCounters[clusterType]) {
                this.clusterTypeCounters[clusterType] = 0;
            }
            
            // Get unique index for this cluster type
            const globalIndex = this.clusterTypeCounters[clusterType];
            this.clusterTypeCounters[clusterType]++;
            
            const uniqueLabel = `${clusterType}_${globalIndex}`;
            
            const clusterNode = {
                id: clusterId,
                label: uniqueLabel,
                type: 'cluster',
                clusterType: (cluster.type || 'unknown').toLowerCase(),
                radius: cluster.radius || 0,
                vector: cluster.vector || [0, 0, 0],
                depth: depth,
                parentCluster: parentCluster,
                x: Math.random() * this.width,
                y: Math.random() * this.height,
                fx: null,
                fy: null
            };
            
            this.clusters.push(clusterNode);
            this.nodes.push(clusterNode);
            
            // Process definitions in this cluster
            if (cluster.definitions && Array.isArray(cluster.definitions)) {
                cluster.definitions.forEach((item, itemIndex) => {
                    if (item.symbol) {
                        // This is a definition
                        const defNode = {
                            id: item.symbol,
                            label: item.symbol,
                            type: 'definition',
                            symbol: item.symbol,
                            vector: item.vector || [0, 0, 0],
                            connections: item.connections || 0,
                            commitFrequency: item.vector ? item.vector[0] : 0,
                            clusterFrequency: item.vector ? item.vector[1] : 0,
                            chronicPoint: item.vector ? item.vector[2] : 0,
                            parentCluster: clusterId,
                            depth: depth + 1,
                            x: Math.random() * this.width,
                            y: Math.random() * this.height,
                            fx: null,
                            fy: null
                        };
                        
                        this.nodes.push(defNode);
                        
                        // Create edge from definition to its cluster
                        this.edges.push({
                            source: defNode,
                            target: clusterNode,
                            weight: 0.8,
                            type: 'cluster_membership'
                        });
                        
                        // If the definition has connection data, create edges to other definitions
                        if (item.connected_to && Array.isArray(item.connected_to)) {
                            item.connected_to.forEach(connectedSymbol => {
                                // We'll create these edges after all nodes are processed
                                // Store the connection info for later processing
                                if (!defNode.pendingConnections) {
                                    defNode.pendingConnections = [];
                                }
                                defNode.pendingConnections.push(connectedSymbol);
                            });
                        }
                        
                    } else if (item.type && item.definitions) {
                        // This is a sub-cluster
                        this.processClusterHierarchy([item], clusterId, depth + 1);
                    } else {
                        console.warn('Unknown item type:', item);
                    }
                });
            }
            
            // Create edge to parent cluster
            if (parentCluster) {
                const parentNode = this.nodes.find(n => n.id === parentCluster);
                if (parentNode) {
                    this.edges.push({
                        source: clusterNode,
                        target: parentNode,
                        weight: 0.6,
                        type: 'cluster_hierarchy'
                    });
                }
            }
        });
    }
    
    /**
     * @brief Process pending connections between definition nodes
     * 
     * Creates edges between definition nodes based on their connection data
     * that was stored during the hierarchy processing phase.
     */
    processPendingConnections() {
        let connectionsAdded = 0;
        
        // Create a map of symbol -> node for quick lookup
        const symbolToNode = new Map();
        this.nodes.forEach(node => {
            if (node.type === 'definition' && node.symbol) {
                symbolToNode.set(node.symbol, node);
            }
        });
        
        // Process pending connections for each definition node
        this.nodes.forEach(node => {
            if (node.type === 'definition' && node.pendingConnections) {
                node.pendingConnections.forEach(connectedSymbol => {
                    const targetNode = symbolToNode.get(connectedSymbol);
                    if (targetNode) {
                        // Check if edge already exists (avoid duplicates)
                        const edgeExists = this.edges.some(edge => 
                            (edge.source.id === node.id && edge.target.id === targetNode.id) ||
                            (edge.source.id === targetNode.id && edge.target.id === node.id)
                        );
                        
                        if (!edgeExists) {
                            this.edges.push({
                                source: node,
                                target: targetNode,
                                weight: 0.6, // Default weight for definition connections
                                type: 'definition_connection'
                            });
                            connectionsAdded++;
                        }
                    } else {
                        console.warn(`Could not find target node for connection: ${node.symbol} -> ${connectedSymbol}`);
                    }
                });
                
                // Clean up pending connections
                delete node.pendingConnections;
            }
        });
    }
    
    /**
     * @brief Build performance caches for faster computation
     * 
     * Creates cached lookups for nodes by parent, edges by node, and cluster children
     * to optimize frequent operations during visualization updates.
     */
    buildPerformanceCaches() {
        
        // Cache nodes by parent for faster lookup
        this.computationCache.nodesByParent.clear();
        this.computationCache.edgesByNode.clear();
        this.computationCache.clusterChildren.clear();
        
        // Build node cache by parent
        this.nodes.forEach(node => {
            if (node.parentCluster) {
                if (!this.computationCache.nodesByParent.has(node.parentCluster)) {
                    this.computationCache.nodesByParent.set(node.parentCluster, []);
                }
                this.computationCache.nodesByParent.get(node.parentCluster).push(node);
            }
            
            // Initialize edge cache for this node
            this.computationCache.edgesByNode.set(node.id, []);
        });
        
        // Build edge cache by node for faster connection lookup
        this.edges.forEach(edge => {
            const sourceId = edge.source.id || edge.source;
            const targetId = edge.target.id || edge.target;
            
            if (this.computationCache.edgesByNode.has(sourceId)) {
                this.computationCache.edgesByNode.get(sourceId).push(edge);
            }
            if (this.computationCache.edgesByNode.has(targetId)) {
                this.computationCache.edgesByNode.get(targetId).push(edge);
            }
        });
        
        // Build cluster children count cache
        this.nodes.filter(n => n.type === 'cluster').forEach(cluster => {
            const children = this.computationCache.nodesByParent.get(cluster.id) || [];
            this.computationCache.clusterChildren.set(cluster.id, {
                definitions: children.filter(c => c.type === 'definition').length,
                clusters: children.filter(c => c.type === 'cluster').length
            });
        });
    }
    
    /**
     * @brief Create the D3.js force-directed visualization
     * 
     * Sets up the force simulation, creates visual elements for nodes and edges,
     * and configures interactive behaviors with performance optimizations.
     */
    createVisualization() {
        // Clear existing elements
        this.g.selectAll('*').remove();
        
        // Create force simulation with optimized parameters
        this.simulation = d3.forceSimulation(this.nodes)
            .force('link', d3.forceLink(this.edges)
                .id(d => d.id)
                .distance(d => 50 + (1 - d.weight) * 100)
                .strength(0.3))
            .force('charge', d3.forceManyBody()
                .strength(-200))
            .force('center', d3.forceCenter(this.width / 2, this.height / 2))
            .force('collision', d3.forceCollide()
                .radius(d => this.getNodeRadius(d) + 5))
            .alphaMin(0.01)
            .alphaDecay(0.0228)
            .velocityDecay(0.4);
        
        // Create edges and nodes
        this.createEdges();
        this.createNodes();
        
        // Use requestAnimationFrame with throttling for smooth updates
        let lastUpdateTime = 0;
        this.simulation.on('tick', () => {
            const now = performance.now();
            if (now - lastUpdateTime >= 16) { // ~60fps cap
                requestAnimationFrame(() => this.updatePositions());
                lastUpdateTime = now;
            }
        });
    }
    
    /**
     * @brief Create edge visual elements
     * 
     * Creates SVG line elements for all edges with appropriate styling
     * based on edge weight and type.
     */
    createEdges() {
        this.edgeElements = this.g.append('g')
            .attr('class', 'edges')
            .selectAll('line')
            .data(this.edges)
            .enter().append('line')
            .attr('class', 'edge')
            .attr('stroke-width', d => Math.sqrt(d.weight) * 2)
            .attr('opacity', d => 0.3 + d.weight * 0.7);
    }
    
    /**
     * @brief Create node visual elements and set up interactions
     * 
     * Creates SVG groups containing circles and labels for each node,
     * sets up mouse event handlers for tooltips and highlighting,
     * and configures drag behavior.
     */
    createNodes() {
        this.nodeElements = this.g.append('g')
            .attr('class', 'nodes')
            .selectAll('g')
            .data(this.nodes)
            .enter().append('g')
            .attr('class', 'node-group')
            .style('cursor', 'pointer')
            .style('pointer-events', 'all');
        
        // Add circles FIRST before adding drag behavior
        this.nodeElements.append('circle')
            .attr('class', d => `node ${d.type}`)
            .attr('r', d => this.getNodeRadius(d))
            .style('pointer-events', 'all');
        
        // Add labels
        this.nodeElements.append('text')
            .attr('class', 'node-label')
            .attr('dy', '.35em')
            .attr('text-anchor', 'middle')
            .text(d => this.getNodeLabel(d))
            .style('font-size', d => Math.max(8, this.getNodeRadius(d) / 3) + 'px')
            .style('pointer-events', 'none')
            .style('user-select', 'none');
        
        // Create throttled hover functions for performance
        const throttledMouseOver = this.throttle((event, d) => {
            // Only show connection highlights on mouseover if no node is currently selected
            if (!this.selectedNode) {
                this.showTooltip(event, d);
                this.highlightConnections(d);
            } else {
                // Just show tooltip if a node is selected
                this.showTooltip(event, d);
            }
        }, 16);
        
        const throttledMouseOut = this.throttle((event, d) => {
            this.hideTooltip();
            
            // Only clear highlights if no node is selected
            if (!this.selectedNode) {
                this.clearHighlights();
            }
        }, 16);
        
        // Add ALL event handlers to the node group BEFORE adding drag behavior
        this.nodeElements
            .on('click', (event, d) => {
                // Prevent click if node was being dragged
                if (d.isDragging) {
                    console.log('Node group click ignored - node was being dragged');
                    return;
                }
                console.log('Node group clicked:', d);
                console.log('Click event coordinates:', event.clientX, event.clientY);
                console.log('Node position:', d.x, d.y);
                console.log('Node transform:', d3.select(event.currentTarget).attr('transform'));
                
                // Clear any existing highlights first
                this.clearHighlights();
                
                // Select the node
                this.selectNode(d);
                
                // For cluster nodes, highlight only the cluster and its direct children
                if (d.type === 'cluster') {
                    console.log('Cluster clicked, calling highlightClusterOnly for:', d.id);
                    this.highlightClusterOnly(d);
                } else {
                    console.log('Non-cluster clicked:', d.type, d.id);
                }
                
                event.stopPropagation();
            })
            .on('mouseover', throttledMouseOver)
            .on('mouseout', throttledMouseOut);
        
        // Add drag behavior LAST so it doesn't interfere with other events
        this.nodeElements.call(this.getDragBehavior());
    }
    
    updatePositions() {
        this.edgeElements
            .attr('x1', d => d.source.x)
            .attr('y1', d => d.source.y)
            .attr('x2', d => d.target.x)
            .attr('y2', d => d.target.y);
        
        this.nodeElements
            .attr('transform', d => `translate(${d.x},${d.y})`);
    }
    
    getNodeRadius(node) {
        if (node.type === 'definition') {
            return 8 + (node.commitFrequency || 0) * 15;
        } else if (node.type === 'cluster') {
            return 15 + (node.radius || 0) * 50;
        }
        return 12;
    }
    
    getNodeLabel(node) {
        if (node.type === 'definition') {
            // For definitions, show the symbol name (truncated if too long)
            let label = node.label || node.symbol || node.id;
            return label;
        } else if (node.type === 'cluster') {
            // For clusters, show the unique label we created
            return node.label || node.clusterType || node.id;
        } else {
            // Fallback for other node types
            let label = node.label || node.id;
            return label;
        }
    }
    
    getDragBehavior() {
        let dragStarted = false;
        
        return d3.drag()
            .on('start', (event, d) => {
                if (!event.active) this.simulation.alphaTarget(0.3).restart();
                d.fx = d.x;
                d.fy = d.y;
                dragStarted = false;
                d.isDragging = false;
            })
            .on('drag', (event, d) => {
                if (!dragStarted) {
                    dragStarted = true;
                    d.isDragging = true;
                }
                d.fx = event.x;
                d.fy = event.y;
            })
            .on('end', (event, d) => {
                if (!event.active) this.simulation.alphaTarget(0);
                d.fx = null;
                d.fy = null;
                
                // Reset dragging flag after a short delay to allow click to complete
                setTimeout(() => {
                    d.isDragging = false;
                    dragStarted = false;
                }, 100);
            });
    }
    
    /**
     * @brief Clear the current node selection
     * 
     * Removes visual selection indicators, clears highlights, and resets
     * the node details panel to its default state.
     */
    clearSelection() {
        // Clear previous selection
        this.g.selectAll('.node').classed('selected', false);
        this.selectedNode = null;
        
        // Clear any highlights
        this.clearHighlights();
        
        // Reset node details to default
        const detailsDiv = this.domCache.nodeDetails || document.getElementById('node-details');
        if (detailsDiv) {
            detailsDiv.innerHTML = '<p>Click on a node to see details</p>';
        }
    }
    
    /**
     * @brief Select a node and update the interface
     * @param {Object} node - The node object to select
     * 
     * Marks the specified node as selected, updates visual indicators,
     * and refreshes the node details panel with the node's information.
     */
    selectNode(node) {
        // Clear previous selection
        this.g.selectAll('.node').classed('selected', false);
        
        // Select new node
        const selectedElements = this.g.selectAll('.node')
            .filter(d => d.id === node.id)
            .classed('selected', true);
        
        this.selectedNode = node;
        
        // Check if node-details element exists
        const detailsDiv = this.domCache.nodeDetails || document.getElementById('node-details');
        if (!detailsDiv) {
            console.error('node-details element not found in DOM!');
            return;
        }
        
        this.updateNodeDetails(node);
    }
    
    /**
     * @brief Update the node details panel with information about the selected node
     * @param {Object} node - The node object to display details for
     * 
     * Generates and displays comprehensive information about the node including
     * its properties, relationships, and metrics in the details panel.
     */
    updateNodeDetails(node) {
        // Use cached DOM element for better performance
        const detailsDiv = this.domCache.nodeDetails || document.getElementById('node-details');
        if (!detailsDiv) {
            console.error('node-details element not found!');
            return;
        }
        
        if (node.type === 'definition') {
            const htmlContent = `
                <h3>Definition Node</h3>
                <div class="node-info"><strong>Symbol:</strong> ${node.symbol || node.label}</div>
                <div class="node-info"><strong>Type:</strong> ${node.type}</div>
                <div class="node-info"><strong>ID:</strong> ${node.id}</div>
                <div class="node-info"><strong>Commit Frequency:</strong> ${(node.commitFrequency || 0).toFixed(6)}</div>
                <div class="node-info"><strong>Cluster Frequency:</strong> ${(node.clusterFrequency || 0).toFixed(6)}</div>
                <div class="node-info"><strong>Chronic Point:</strong> ${(node.chronicPoint || 0).toFixed(6)}</div>
                <div class="node-info"><strong>Connections:</strong> ${node.connections || 0}</div>
                <div class="node-info"><strong>Vector:</strong> [${(node.vector || []).map(v => v.toFixed(6)).join(', ')}]</div>
                <div class="node-info"><strong>Parent Cluster:</strong> ${node.parentCluster || 'None'}</div>
                <div class="node-info"><strong>Depth:</strong> ${node.depth || 0}</div>
                <div class="node-info"><strong>Position:</strong> (${node.x ? node.x.toFixed(2) : 'N/A'}, ${node.y ? node.y.toFixed(2) : 'N/A'})</div>
            `;
            detailsDiv.innerHTML = htmlContent;
        } else if (node.type === 'cluster') {
            // Use cached children for better performance
            const cachedChildren = this.computationCache.clusterChildren.get(node.id) || { definitions: 0, clusters: 0 };
            const childDefinitions = this.computationCache.nodesByParent.get(node.id)?.filter(n => n.type === 'definition') || [];
            
            const htmlContent = `
                <h3>Cluster Node</h3>
                <div class="node-info"><strong>Cluster Type:</strong> ${node.clusterType}</div>
                <div class="node-info"><strong>Label:</strong> ${node.label}</div>
                <div class="node-info"><strong>ID:</strong> ${node.id}</div>
                <div class="node-info"><strong>Radius:</strong> ${(node.radius || 0).toFixed(6)}</div>
                <div class="node-info"><strong>Depth:</strong> ${node.depth}</div>
                <div class="node-info"><strong>Vector:</strong> [${(node.vector || []).map(v => v.toFixed(6)).join(', ')}]</div>
                <div class="node-info"><strong>Parent:</strong> ${node.parentCluster || 'Root'}</div>
                <div class="node-info"><strong>Child Definitions:</strong> ${cachedChildren.definitions}</div>
                <div class="node-info"><strong>Child Clusters:</strong> ${cachedChildren.clusters}</div>
                <div class="node-info"><strong>Position:</strong> (${node.x ? node.x.toFixed(2) : 'N/A'}, ${node.y ? node.y.toFixed(2) : 'N/A'})</div>
                ${childDefinitions.length > 0 ? `
                <div class="node-info"><strong>Definitions:</strong></div>
                <ul style="margin-left: 20px; font-size: 12px;">
                    ${childDefinitions.slice(0, 10).map(def => `<li>${def.symbol}</li>`).join('')}
                    ${childDefinitions.length > 10 ? `<li><em>... and ${childDefinitions.length - 10} more</em></li>` : ''}
                </ul>
                ` : ''}
            `;
            detailsDiv.innerHTML = htmlContent;
        } else {
            const htmlContent = `
                <h3>Unknown Node Type</h3>
                <div class="node-info"><strong>ID:</strong> ${node.id}</div>
                <div class="node-info"><strong>Type:</strong> ${node.type}</div>
                <div class="node-info"><strong>Label:</strong> ${node.label}</div>
                <div class="node-info"><strong>Position:</strong> (${node.x ? node.x.toFixed(2) : 'N/A'}, ${node.y ? node.y.toFixed(2) : 'N/A'})</div>
                <div class="node-info"><strong>Raw Data:</strong></div>
                <pre style="font-size: 10px; background: #f5f5f5; padding: 10px; border-radius: 4px; max-height: 200px; overflow-y: auto;">${JSON.stringify(node, null, 2)}</pre>
            `;
            detailsDiv.innerHTML = htmlContent;
        }
    }
    
    /**
     * @brief Show tooltip with node information on hover
     * @param {Event} event - The mouse event containing position information
     * @param {Object} node - The node object to display tooltip for
     * 
     * Displays a tooltip with concise node information at the mouse position.
     * Optimized to avoid rebuilding content if the same node is hovered.
     */
    showTooltip(event, node) {
        // Use cached DOM element for better performance
        const tooltip = this.domCache.tooltip || document.getElementById('tooltip');
        if (!tooltip) {
            console.error('Tooltip element not found!');
            return;
        }
        
        // Early exit if tooltip is already showing for this node
        if (this.currentTooltipNode === node.id) {
            // Just update position without rebuilding content
            tooltip.style.left = (event.clientX + 10) + 'px';
            tooltip.style.top = (event.clientY - 10) + 'px';
            return;
        }
        
        let content = `<strong>${this.getNodeLabel(node)}</strong><br>`;
        content += `Type: ${node.type}<br>`;
        content += `ID: ${node.id}<br>`;
        
        if (node.type === 'definition') {
            content += `Symbol: ${node.symbol}<br>`;
            content += `Commit Freq: ${(node.commitFrequency || 0).toFixed(3)}<br>`;
            content += `Chronic Point: ${(node.chronicPoint || 0).toFixed(3)}<br>`;
            content += `Connections: ${node.connections || 0}<br>`;
            content += `Parent: ${node.parentCluster || 'None'}`;
        } else if (node.type === 'cluster') {
            content += `Cluster Type: ${node.clusterType}<br>`;
            content += `Radius: ${(node.radius || 0).toFixed(3)}<br>`;
            content += `Depth: ${node.depth}<br>`;
            content += `Parent: ${node.parentCluster || 'Root'}`;
            
            // Use cached child count for performance
            const cachedChildren = this.computationCache.clusterChildren.get(node.id);
            if (cachedChildren && (cachedChildren.definitions > 0 || cachedChildren.clusters > 0)) {
                content += `<br>Children: ${cachedChildren.definitions + cachedChildren.clusters}`;
            }
        }
        
        content += `<br><em>Click for full details</em>`;
        
        tooltip.innerHTML = content;
        
        // Use the correct event position for tooltip
        tooltip.style.left = (event.clientX + 10) + 'px';
        tooltip.style.top = (event.clientY - 10) + 'px';
        tooltip.classList.add('visible');
        
        // Track current tooltip node to avoid redundant updates
        this.currentTooltipNode = node.id;
    }
    
    /**
     * @brief Hide the tooltip
     * 
     * Removes the tooltip from view and resets tracking variables.
     */
    hideTooltip() {
        const tooltip = this.domCache.tooltip || document.getElementById('tooltip');
        if (tooltip) {
            tooltip.classList.remove('visible');
            this.currentTooltipNode = null; // Reset tracking
        } else {
            console.error('Tooltip element not found when trying to hide!');
        }
    }
    
    /**
     * @brief Highlight connections for the specified node
     * @param {Object} node - The node whose connections should be highlighted
     * 
     * Visually emphasizes the edges and connected nodes for the given node,
     * using cached data for performance optimization.
     */
    highlightConnections(node) {
        // Early exit if we already have highlights to avoid redundant work
        if (this.currentHighlightedNode === node.id) {
            return;
        }
        
        // Use cached edges for performance
        const cachedEdges = this.computationCache.edgesByNode.get(node.id) || [];
        
        // Early exit if no connections
        if (cachedEdges.length === 0) {
            this.currentHighlightedNode = node.id;
            return;
        }
        
        const connectedNodes = new Set([node.id]); // Include the node itself
        const connectedEdges = new Set();
        
        // Use cached edges instead of iterating through all edges
        cachedEdges.forEach(edge => {
            connectedEdges.add(edge);
            connectedNodes.add(edge.source.id || edge.source);
            connectedNodes.add(edge.target.id || edge.target);
        });
        
        // Use more efficient DOM operations with fewer selections
        this.g.selectAll('.edge')
            .classed('highlighted', d => connectedEdges.has(d));
        
        this.g.selectAll('.node')
            .style('opacity', d => connectedNodes.has(d.id) ? 1 : 0.3);
        
        // Track what we just highlighted to avoid redundant work
        this.currentHighlightedNode = node.id;
    }
    
    /**
     * @brief Highlight only the cluster hierarchy for the specified cluster
     * @param {Object} clusterNode - The cluster node to highlight with its hierarchy
     * 
     * Highlights the specified cluster and all its descendant nodes (both clusters
     * and definitions) with depth-based color coding for visual hierarchy representation.
     */
    highlightClusterOnly(clusterNode) {
        const relatedNodes = new Map(); // Changed to Map to store depth info
        const relatedEdges = new Set();
        
        // Add the cluster itself at depth 0 (root of selection)
        relatedNodes.set(clusterNode.id, 0);
        
        // Recursively find all descendant clusters and definitions
        this.findDescendantNodes(clusterNode.id, 0, relatedNodes);
        
        // Find edges that connect nodes in the hierarchy
        this.edges.forEach(edge => {
            const sourceInHierarchy = relatedNodes.has(edge.source.id);
            const targetInHierarchy = relatedNodes.has(edge.target.id);
            
            // Include cluster_membership and cluster_hierarchy edges within the hierarchy
            if ((edge.type === 'cluster_membership' || edge.type === 'cluster_hierarchy') && 
                sourceInHierarchy && targetInHierarchy) {
                relatedEdges.add(edge);
            }
        });
        
        // Calculate max depth for color scaling
        const maxDepth = Math.max(...relatedNodes.values());
        
        // Highlight edges
        this.g.selectAll('.edge')
            .classed('highlighted', d => relatedEdges.has(d));
        
        // Apply depth-based coloring and opacity to nodes
        this.g.selectAll('.node')
            .style('opacity', d => relatedNodes.has(d.id) ? 1 : 0.3)
            .style('fill', d => {
                if (relatedNodes.has(d.id)) {
                    const depth = relatedNodes.get(d.id);
                    return this.getDepthColor(depth, maxDepth);
                }
                return null; // Reset to default fill
            })
            .style('stroke', d => {
                if (relatedNodes.has(d.id)) {
                    const depth = relatedNodes.get(d.id);
                    return this.getDepthColor(depth, maxDepth);
                }
                return null;
            })
            .style('stroke-width', d => relatedNodes.has(d.id) ? '3px' : '1px');
        
        // Set tracking flag to indicate we have active highlights
        this.hasActiveHighlights = true;
        this.currentHighlightedNode = clusterNode.id;
    }
    
    /**
     * @brief Recursively find all descendant nodes of a parent cluster
     * @param {string} parentId - ID of the parent cluster
     * @param {number} currentDepth - Current depth in the hierarchy
     * @param {Map} relatedNodes - Map to store related nodes with their depths
     * 
     * Traverses the cluster hierarchy to find all child nodes (clusters and definitions)
     * and stores them with their hierarchical depth information.
     */
    findDescendantNodes(parentId, currentDepth, relatedNodes) {
        // Use cached children for better performance
        const children = this.computationCache.nodesByParent.get(parentId) || [];
        
        children.forEach(node => {
            if (!relatedNodes.has(node.id)) {
                const childDepth = currentDepth + 1;
                relatedNodes.set(node.id, childDepth);
                
                // If this child is a cluster, recursively find its children
                if (node.type === 'cluster') {
                    this.findDescendantNodes(node.id, childDepth, relatedNodes);
                }
            }
        });
    }
    
    /**
     * @brief Generate a color based on hierarchical depth
     * @param {number} depth - The depth level in the hierarchy
     * @param {number} maxDepth - The maximum depth in the current hierarchy
     * @return {string} RGB color string for the specified depth
     * 
     * Creates a color gradient from green (depth 0) to red (max depth)
     * to visually represent hierarchical relationships.
     */
    getDepthColor(depth, maxDepth) {
        // Create a color gradient from green (depth 0) to red (max depth)
        if (maxDepth === 0) {
            return '#00ff00'; // Pure green for single node
        }
        
        // Normalize depth to 0-1 range
        const normalizedDepth = depth / maxDepth;
        
        // Interpolate between green (0, 255, 0) and red (255, 0, 0)
        const red = Math.round(255 * normalizedDepth);
        const green = Math.round(255 * (1 - normalizedDepth));
        const blue = 0;
        
        return `rgb(${red}, ${green}, ${blue})`;
    }
    
    /**
     * @brief Clear all visual highlights and reset to default appearance
     * 
     * Removes highlighting from edges and nodes, resets opacity and colors,
     * and clears tracking variables for optimal performance.
     */
    clearHighlights() {
        // Early exit if nothing is highlighted
        if (!this.currentHighlightedNode && !this.hasActiveHighlights) {
            return;
        }
        
        // Use efficient batch operations
        this.g.selectAll('.edge').classed('highlighted', false);
        this.g.selectAll('.node')
            .style('opacity', 1)
            .style('fill', null)
            .style('stroke', null)
            .style('stroke-width', '1px');
        
        // Reset tracking variables
        this.currentHighlightedNode = null;
        this.hasActiveHighlights = false;
    }
    
    /**
     * @brief Center the view to show all visualization content
     * 
     * Calculates the bounding box of all elements and adjusts the zoom/pan
     * to fit the entire visualization within the visible area.
     */
    centerView() {
        const bounds = this.g.node().getBBox();
        const fullWidth = this.width;
        const fullHeight = this.height;
        const width = bounds.width;
        const height = bounds.height;
        const midX = bounds.x + width / 2;
        const midY = bounds.y + height / 2;
        
        if (width == 0 || height == 0) return;
        
        const scale = Math.min(fullWidth / width, fullHeight / height) * 0.9;
        const translate = [fullWidth / 2 - scale * midX, fullHeight / 2 - scale * midY];
        
        this.svg.transition()
            .duration(750)
            .call(
                this.zoom.transform,
                d3.zoomIdentity.translate(translate[0], translate[1]).scale(scale)
            );
    }

    /**
     * @brief Center the view on a specific cluster and its descendants
     * @param {Object} cluster - The cluster object to center the view on
     * 
     * Calculates the bounding box of the cluster and all its descendant nodes,
     * then smoothly transitions the view to focus on that area with appropriate scaling.
     */
    centerOnCluster(cluster) {
        // Find all nodes related to this cluster (the cluster itself + its descendants)
        const relatedNodes = [cluster]; // Start with the cluster itself
        
        // Add all descendant nodes (definitions and sub-clusters)
        const descendants = this.findAllDescendants(cluster.id);
        relatedNodes.push(...descendants);
        
        if (relatedNodes.length === 0) {
            console.warn('No nodes found for cluster:', cluster.id);
            return;
        }
        
        // Calculate bounding box for all related nodes
        let minX = Infinity, maxX = -Infinity;
        let minY = Infinity, maxY = -Infinity;
        
        relatedNodes.forEach(node => {
            if (node.x !== undefined && node.y !== undefined) {
                const radius = this.getNodeRadius(node);
                minX = Math.min(minX, node.x - radius);
                maxX = Math.max(maxX, node.x + radius);
                minY = Math.min(minY, node.y - radius);
                maxY = Math.max(maxY, node.y + radius);
            }
        });
        
        // Check if we have valid bounds
        if (minX === Infinity || maxX === -Infinity || minY === Infinity || maxY === -Infinity) {
            console.warn('Could not calculate bounds for cluster nodes');
            return;
        }
        
        // Add some padding around the cluster
        const padding = 50;
        minX -= padding;
        maxX += padding;
        minY -= padding;
        maxY += padding;
        
        const clusterWidth = maxX - minX;
        const clusterHeight = maxY - minY;
        const centerX = minX + clusterWidth / 2;
        const centerY = minY + clusterHeight / 2;
        
        // Calculate scale to fit the cluster in the view
        const scaleX = this.width / clusterWidth;
        const scaleY = this.height / clusterHeight;
        const scale = Math.min(scaleX, scaleY, 2.0) * 0.8; // Cap at 2x zoom and add some margin
        
        // Calculate translation to center the cluster
        const translateX = this.width / 2 - scale * centerX;
        const translateY = this.height / 2 - scale * centerY;
        
        // Apply the transformation with smooth transition
        this.svg.transition()
            .duration(1000)
            .ease(d3.easeQuadInOut)
            .call(
                this.zoom.transform,
                d3.zoomIdentity.translate(translateX, translateY).scale(scale)
            );
    }

    /**
     * @brief Find all descendant nodes of a parent cluster
     * @param {string} parentId - ID of the parent cluster
     * @return {Array} Array of all descendant nodes (clusters and definitions)
     * 
     * Recursively traverses the cluster hierarchy to collect all child nodes.
     */
    findAllDescendants(parentId) {
        const descendants = [];
        const children = this.computationCache.nodesByParent.get(parentId) || [];
        
        children.forEach(child => {
            descendants.push(child);
            
            // If this child is a cluster, recursively find its descendants
            if (child.type === 'cluster') {
                descendants.push(...this.findAllDescendants(child.id));
            }
        });
        
        return descendants;
    }
    
    displayClusters() {
        // This would display cluster boundaries if we had spatial cluster data
        console.log('Cluster display functionality would go here');
    }
    
    hideClusters() {
        this.g.selectAll('.cluster-boundary').remove();
    }
    
    addAllDescendants(parentId, allVisibleNodes, matchingClusters) {
        // Use cached children for better performance
        const children = this.computationCache.nodesByParent.get(parentId) || [];
        
        children.forEach(child => {
            if (!allVisibleNodes.has(child.id)) {
                allVisibleNodes.add(child.id);
                console.log('Added descendant:', child.id, 'type:', child.type, 
                          child.type === 'cluster' ? `cluster type: ${child.clusterType}` : '');
                
                // If this child is a cluster, add it to matching clusters and recurse
                if (child.type === 'cluster') {
                    matchingClusters.add(child.id);
                    this.addAllDescendants(child.id, allVisibleNodes, matchingClusters);
                }
            }
        });
    }
    
    /**
     * @brief Filters the visualization to show only clusters of a specific type and their hierarchies
     * @param {string} filterType - The cluster type to filter by ('all' to show everything)
     * @details This function hides/shows clusters and definitions based on the filter type.
     *          When filtering by a specific type, it shows the matching clusters and all their
     *          descendants (both sub-clusters and definitions). Visual styling is applied to
     *          distinguish between root matching clusters and child clusters.
     */
    filterClusters(filterType) {
        this.currentFilter = filterType;
        
        if (filterType === 'all') {
            // Show all node groups (this includes both circles and labels)
            this.g.selectAll('.node-group').style('display', 'block');
            this.g.selectAll('.edge').style('display', 'block');
            
            // Reset any special styling
            this.g.selectAll('.node-group')
                .select('circle')
                .style('stroke', null)
                .style('stroke-width', '1px');
        } else {
            // Hide all node groups first (this hides both circles and labels)
            this.g.selectAll('.node-group').style('display', 'none');
            this.g.selectAll('.edge').style('display', 'none');
            
            // Collect IDs of visible nodes for edge filtering
            const visibleNodeIds = new Set();
            
            // First, collect all clusters that match the filter type and their descendants
            const matchingClusters = new Set();
            const allVisibleNodes = new Set();
            
            // Find root clusters that match the filter type
            this.nodes.filter(n => n.type === 'cluster' && n.clusterType === filterType).forEach(cluster => {
                matchingClusters.add(cluster.id);
                allVisibleNodes.add(cluster.id);
                
                // Recursively add all descendants (clusters and definitions)
                this.addAllDescendants(cluster.id, allVisibleNodes, matchingClusters);
            });
            
            // Show node groups that match the filter or are descendants
            this.g.selectAll('.node-group')
                .filter(d => {
                    const shouldShow = allVisibleNodes.has(d.id);
                    if (shouldShow) {
                        visibleNodeIds.add(d.id);
                    }
                    return shouldShow;
                })
                .style('display', 'block');
            
            // Add visual distinction between root matches and child clusters
            this.g.selectAll('.node-group')
                .filter(d => allVisibleNodes.has(d.id))
                .select('circle')
                .style('stroke', d => {
                    if (d.type === 'cluster') {
                        if (d.clusterType === filterType) {
                            return '#ff6b35'; // Orange border for root matching clusters
                        } else {
                            return '#4ecdc4'; // Teal border for child clusters
                        }
                    }
                    return '#666'; // Default for definitions
                })
                .style('stroke-width', d => {
                    if (d.type === 'cluster') {
                        return d.clusterType === filterType ? '4px' : '2px';
                    }
                    return '1px';
                });
            
            // Show edges that connect visible nodes
            this.g.selectAll('.edge')
                .filter(d => {
                    // Handle both object references and ID references
                    const sourceId = (typeof d.source === 'object') ? d.source.id : d.source;
                    const targetId = (typeof d.target === 'object') ? d.target.id : d.target;
                    
                    return visibleNodeIds.has(sourceId) && visibleNodeIds.has(targetId);
                })
                .style('display', 'block');
        }
        
        this.updateStatistics();
        
        // Update legend with current filter info
        if (filterType !== 'all') {
            const matchingClusters = new Set();
            const allVisibleNodes = new Set();
            
            // Recalculate for legend (we could optimize this by storing the values)
            this.nodes.filter(n => n.type === 'cluster' && n.clusterType === filterType).forEach(cluster => {
                matchingClusters.add(cluster.id);
                allVisibleNodes.add(cluster.id);
                this.addAllDescendants(cluster.id, allVisibleNodes, matchingClusters);
            });
            
            this.updateFilterLegend(filterType, matchingClusters, allVisibleNodes);
        } else {
            this.updateFilterLegend(filterType, null, null);
        }
        
        // Update cluster list to show filter state
        this.updateClusterList();
    }
    
    updateFilterLegend(filterType, matchingClusters, allVisibleNodes) {
        const statsPanel = document.getElementById('stats-panel');
        if (!statsPanel) return;
        
        // Remove existing legend
        const existingLegend = statsPanel.querySelector('.filter-legend');
        if (existingLegend) {
            existingLegend.remove();
        }
        
        if (filterType !== 'all' && matchingClusters && allVisibleNodes) {
            const legend = document.createElement('div');
            legend.className = 'filter-legend';
            legend.style.cssText = `
                margin-top: 15px;
                padding: 10px;
                background: #f8f9fa;
                border-radius: 4px;
                border-left: 3px solid #007bff;
                font-size: 12px;
            `;
            
            const rootCount = Array.from(matchingClusters).filter(id => {
                const node = this.nodes.find(n => n.id === id);
                return node && node.clusterType === filterType;
            }).length;
            
            const childCount = matchingClusters.size - rootCount;
            const definitionCount = allVisibleNodes.size - matchingClusters.size;
            
            legend.innerHTML = `
                <h4 style="margin: 0 0 8px 0; color: #007bff;">Filter: ${filterType.toUpperCase()}</h4>
                <div style="margin: 4px 0; display: flex; align-items: center;">
                    <div style="width: 12px; height: 12px; border: 3px solid #ff6b35; border-radius: 50%; margin-right: 8px;"></div>
                    <span>Root ${filterType} clusters: ${rootCount}</span>
                </div>
                <div style="margin: 4px 0; display: flex; align-items: center;">
                    <div style="width: 12px; height: 12px; border: 2px solid #4ecdc4; border-radius: 50%; margin-right: 8px;"></div>
                    <span>Child clusters: ${childCount}</span>
                </div>
                <div style="margin: 4px 0; display: flex; align-items: center;">
                    <div style="width: 12px; height: 12px; border: 1px solid #666; border-radius: 50%; margin-right: 8px;"></div>
                    <span>Definitions: ${definitionCount}</span>
                </div>
            `;
            
            statsPanel.appendChild(legend);
        }
    }
    
    /**
     * @brief Updates the statistics display with current node and edge counts
     * @details Counts visible nodes and edges by checking their display style
     *          and updates the DOM elements showing the statistics.
     */
    updateStatistics() {
        // Use cached DOM elements for better performance
        if (!this.domCache.totalNodes) this.cacheDOMElements();
        
        // Count visible nodes by checking node groups
        let visibleNodes = 0;
        let visibleEdges = 0;
        
        this.g.selectAll('.node-group').each(function() {
            if (d3.select(this).style('display') !== 'none') {
                visibleNodes++;
            }
        });
        
        this.g.selectAll('.edge').each(function() {
            if (d3.select(this).style('display') !== 'none') {
                visibleEdges++;
            }
        });
        
        const clusterCount = this.nodes.filter(n => n.type === 'cluster').length;
        
        // Use cached DOM elements
        if (this.domCache.totalNodes) this.domCache.totalNodes.textContent = this.nodes.length;
        if (this.domCache.totalEdges) this.domCache.totalEdges.textContent = this.edges.length;
        if (this.domCache.totalClusters) this.domCache.totalClusters.textContent = clusterCount;
        if (this.domCache.visibleNodes) this.domCache.visibleNodes.textContent = visibleNodes;
    }

    /**
     * @brief Updates the cluster list UI with currently visible clusters
     * @details Populates the cluster list panel with cluster information, applying
     *          appropriate filtering and styling based on the current filter state.
     *          Adds click handlers for cluster selection and visual indicators for
     *          hierarchy relationships when filtering is active.
     */
    updateClusterList() {
        const clusterList = document.getElementById('cluster-list');
        clusterList.innerHTML = '';
        
        // Filter to only show cluster nodes (not definition nodes)
        let clusterNodes = this.clusters.filter(n => n.type === 'cluster');
        
        // Check if we're currently filtering
        const isFiltering = this.currentFilter && this.currentFilter !== 'all';
        let visibleClusterIds = new Set();
        
        if (isFiltering) {
            // Calculate which clusters are currently visible
            this.g.selectAll('.node-group').each(function(d) {
                if (d3.select(this).style('display') !== 'none' && d.type === 'cluster') {
                    visibleClusterIds.add(d.id);
                }
            });
            
            // Filter cluster nodes to only show visible ones
            clusterNodes = clusterNodes.filter(cluster => visibleClusterIds.has(cluster.id));
        }
        
        clusterNodes.forEach((cluster, index) => {
            const clusterItem = document.createElement('div');
            
            // Apply different styling based on filter status
            let className = `cluster-item ${cluster.clusterType}`;
            let isRootMatch = isFiltering && cluster.clusterType === this.currentFilter;
            let isChildCluster = isFiltering && !isRootMatch;
            
            if (isFiltering) {
                if (isRootMatch) {
                    className += ' root-match';
                } else if (isChildCluster) {
                    className += ' child-cluster';
                }
            }
            
            clusterItem.className = className;
            clusterItem.style.cursor = 'pointer'; // Make it clear the item is clickable
            clusterItem.style.transition = 'background-color 0.2s'; // Add hover effect
            
            // Add hover effect
            clusterItem.addEventListener('mouseenter', () => {
                clusterItem.style.backgroundColor = '#f0f8ff';
            });
            clusterItem.addEventListener('mouseleave', () => {
                clusterItem.style.backgroundColor = '';
            });
            
            // Count definitions in this cluster by exact cluster ID match
            const definitionCount = this.nodes.filter(n => 
                n.type === 'definition' && n.parentCluster === cluster.id
            ).length;
            
            // Add visual indicator for hierarchy when filtering
            let hierarchyIndicator = '';
            if (isFiltering) {
                if (isRootMatch) {
                    hierarchyIndicator = '<span style="color: #ff6b35; font-weight: bold;">🎯 </span>';
                } else if (isChildCluster) {
                    hierarchyIndicator = '<span style="color: #4ecdc4; font-weight: bold;">↳ </span>';
                }
            }
            
            clusterItem.innerHTML = `
                ${hierarchyIndicator}<div class="cluster-title">${cluster.label} (${cluster.clusterType.toUpperCase()})</div>
                <div class="cluster-info">Radius: ${(cluster.radius || 0).toFixed(4)}</div>
                <div class="cluster-info">Definitions: ${definitionCount}</div>
                <div class="cluster-info">Depth: ${cluster.depth}</div>
                <div class="cluster-info">ID: ${cluster.id}</div>
            `;
            
            clusterItem.addEventListener('click', () => {
                this.selectCluster(cluster);
            });
            
            clusterList.appendChild(clusterItem);
        });
        
        // Add a summary message when filtering
        if (isFiltering && clusterNodes.length > 0) {
            const summaryDiv = document.createElement('div');
            summaryDiv.style.cssText = `
                margin-top: 10px;
                padding: 8px;
                background: #e8f4f8;
                border-radius: 4px;
                font-size: 12px;
                color: #2c3e50;
                text-align: center;
                font-style: italic;
            `;
            summaryDiv.innerHTML = `Showing ${clusterNodes.length} clusters from ${this.currentFilter.toUpperCase()} hierarchy`;
            clusterList.appendChild(summaryDiv);
        } else if (isFiltering && clusterNodes.length === 0) {
            const noResultsDiv = document.createElement('div');
            noResultsDiv.style.cssText = `
                margin-top: 10px;
                padding: 15px;
                background: #fff3cd;
                border-radius: 4px;
                font-size: 14px;
                color: #856404;
                text-align: center;
            `;
            noResultsDiv.innerHTML = `No clusters found for ${this.currentFilter.toUpperCase()}`;
            clusterList.appendChild(noResultsDiv);
        }
    }
    
    /**
     * @brief Selects and highlights a cluster from the cluster list
     * @param {Object} cluster - The cluster object to select
     * @details Clears existing highlights, selects the cluster node, applies cluster-only
     *          highlighting, and centers the view on the selected cluster.
     */
    selectCluster(cluster) {
        // Clear any existing highlights first
        this.clearHighlights();
        
        // Select the cluster node (same as clicking on it)
        this.selectNode(cluster);
        
        // Use the same highlighting logic as clicking on the cluster directly
        this.highlightClusterOnly(cluster);
        
        // Center the view on this cluster and its content
        this.centerOnCluster(cluster);
    }

    /**
     * @brief Highlights a cluster and its related nodes and edges
     * @param {Object} cluster - The cluster to highlight
     * @details Highlights the cluster node and all its definitions, dimming other nodes
     *          and highlighting related edges for better visual focus.
     */
    highlightCluster(cluster) {
        // Clear previous highlights
        this.clearHighlights();
        
        // Highlight the cluster
        this.g.selectAll('.node')
            .filter(d => d.label === cluster.label)
            .classed('selected', true);
        
        // Highlight all definitions in this cluster
        const relatedNodes = this.nodes.filter(n => 
            n.parentCluster === cluster.id || n.id === cluster.id
        );
        
        this.g.selectAll('.node')
            .style('opacity', d => relatedNodes.some(rn => rn.id === d.id) ? 1 : 0.3);
        
        // Highlight related edges
        this.g.selectAll('.edge')
            .classed('highlighted', d => 
                relatedNodes.some(rn => rn.id === d.source.id) || 
                relatedNodes.some(rn => rn.id === d.target.id)
            );
    }

    /**
     * @brief Handles window resize events
     * @details Updates SVG dimensions and restarts the force simulation with new center
     *          coordinates to maintain proper layout after resize.
     */
    handleResize() {
        this.width = window.innerWidth - 300;
        this.height = window.innerHeight - 80;
        
        this.svg
            .attr('width', this.width)
            .attr('height', this.height);
        
        if (this.simulation) {
            this.simulation
                .force('center', d3.forceCenter(this.width / 2, this.height / 2))
                .restart();
        }
    }

    /**
     * @brief Shows or hides the loading indicator
     * @param {boolean} show - Whether to show (true) or hide (false) the loading indicator
     * @details Uses cached DOM elements for better performance when toggling the loading state.
     */
    showLoading(show) {
        // Use cached DOM element for better performance
        if (!this.domCache.loading) this.cacheDOMElements();
        
        const loading = this.domCache.loading || document.getElementById('loading');
        if (loading) {
            if (show) {
                loading.classList.remove('hidden');
            } else {
                loading.classList.add('hidden');
            }
        }
    }

    /**
     * @brief Displays an error message to the user
     * @param {string} message - The error message to display
     * @details Creates a modal-style error dialog with the message and logs the error
     *          to the console for debugging purposes.
     */
    showError(message) {
        console.error('Showing error:', message);
        
        // Create a more detailed error display
        const errorDiv = document.createElement('div');
        errorDiv.style.cssText = `
            position: fixed;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background: #dc3545;
            color: white;
            padding: 2rem;
            border-radius: 8px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.3);
            z-index: 3000;
            max-width: 500px;
            text-align: center;
        `;
        
        errorDiv.innerHTML = `
            <h3>❌ Error Loading Data</h3>
            <p>${message}</p>
            <p><small>Check the browser console (F12) for detailed error information.</small></p>
            <button onclick="this.parentElement.remove()" style="margin-top: 1rem; background: white; color: #dc3545; border: none; padding: 0.5rem 1rem; border-radius: 4px; cursor: pointer;">Close</button>
        `;
        
        document.body.appendChild(errorDiv);
        
        // Also show in console
        console.error('DMC Visualizer Error:', message);
    }
}

// Initialize the visualizer when the page loads
document.addEventListener('DOMContentLoaded', () => {
    new DMCVisualizer();
});
