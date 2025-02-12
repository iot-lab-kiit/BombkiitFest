#include <ESPAsyncWebServer.h>

#include <AsyncTCP.h>

AsyncWebServer server(80);

volatile int isSolved = 0;

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>QR Code Puzzle</title>
      <link rel="stylesheet" href="styles.css">
  </head>
  <body>
      <div id="container">
          <div id="puzzle-container"></div>
          <button id="shuffle-button">Shuffle</button>
          <div id="message"></div>
          <div id="author">
              <p>Created by SUMAN SAHA</p>
              <p>GitHub: <a href="https://github.com/circuito-suman/PuzzleGame">https://github.com/circuito-suman/PuzzleGame</a></p>
              <p>Share with others: <a href="https://circuito-suman.github.io/PuzzleGame/">https://circuito-suman.github.io/PuzzleGame/</a></p>

          </div>
      </div>
      <script src="https://cdn.jsdelivr.net/npm/qrcode/build/qrcode.min.js"></script>
      <script src="script.js"></script>
  </body>
  </html>

  )rawliteral";

// CSS content
const char styles_css[] PROGMEM = R"rawliteral(body {
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
      background: teal url(https://assets.codepen.io/15664/bliss.jpg) center / cover no-repeat;
      
  }

  #container {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
  }

  #puzzle-container {
      display: grid;
      grid-template-columns: repeat(3, 100px); /* Changed to 3 columns */
      grid-template-rows: repeat(3, 100px);    /* Changed to 3 rows */
      gap: 2px;
      margin-bottom: 20px;
      user-select: none;
      position: relative;
      width: 300px;
      height: 300px;
      border: 2px solid #0026ff;
      box-shadow: 0 0 10px rgba(0, 0, 0);

  }

  .puzzle-piece {
      width: 100px;  /* Changed to 100px */
      height: 100px; /* Changed to 100px */
      background-color: white;
      border: 1px solid #ccc;
      box-sizing: border-box;
      cursor: pointer;
      position: absolute;
      background-size: 300px 300px;
      transition: all 0.2s ease;
  }

  .puzzle-piece:hover {
      transform: scale(1.05);
      z-index: 1;
  }

  .hidden {
      opacity: 0;
      pointer-events: none;
  }

  #shuffle-button {
      font-size: medium;
      font-family: Georgia, 'Times New Roman', Times, serif;
      font-weight: bold;
      padding: 10px;
      border-radius: 25%;
      background-color: #002fff;
      color: white;
      cursor: pointer;
      margin-bottom: 10px;
  }

  #message {
      margin-top: 20px;
      font-weight: bolder;
      font-size: 1.5em;
      color: rgb(255, 255, 255);
      text-align: center;
  }


  #author {
      display: inline;
      margin-top: 20px;
      font-size: 16px;
      color: #d40000;
      text-align: center;
      font-weight: bolder;
  }

  #author p {
      margin: 5px 0;
      background-color: #ffffff;
      border-radius: 25%;
      padding: 1px;
      margin: auto;
  }

  #author a {
      color: blue;
      text-decoration: underline;
  }



  )rawliteral";

// JavaScript content
const char script_js[] PROGMEM = R"rawliteral(const qrText = 'https://www.youtube.com/shorts/41iWg91yFv0';
  const qrSize = 300;
  const pieces = 3;  // Changed to 3x3
  const pieceSize = qrSize / pieces; // Will be 100px
  let puzzleContainer, emptyX, emptyY;
  let imageURL;

  // Add these variables at the top with other constants
  let isDragging = false;
  let draggedPiece = null;
  let startX, startY;
  let originalX, originalY;

  function loadImage(url) {
      return new Promise((resolve, reject) => {
          const image = new Image();
          image.onload = () => resolve(image);
          image.onerror = () => reject(new Error('Could not load image'));
          image.crossOrigin = "anonymous"; // Add this to handle CORS
          image.src = url;
      });
  }

  // Replace the existing event listeners in DOMContentLoaded
  document.addEventListener('DOMContentLoaded', async () => {
      puzzleContainer = document.getElementById('puzzle-container');
      const shuffleButton = document.getElementById('shuffle-button');
      shuffleButton.addEventListener('click', shufflePuzzle);
      
      // Update event listeners for drag functionality
      puzzleContainer.addEventListener('mousedown', startDragging);
      document.addEventListener('mousemove', dragPiece);
      document.addEventListener('mouseup', stopDragging);
      
      // Touch events
      puzzleContainer.addEventListener('touchstart', handleTouchStart, { passive: false });
      document.addEventListener('touchmove', handleTouchMove, { passive: false });
      document.addEventListener('touchend', handleTouchEnd);

      try {
          imageURL = "https://i.postimg.cc/wjbYC3Ty/KIITFEST-8-0-LOGO.jpg";
          const image = await loadImage(imageURL);
          createPuzzle(image);
      } catch (error) {
          console.error('Error loading image:', error);
          document.getElementById('message').innerText = 'Failed to load image';
      }
  });

  let correctPieceOrder = [];

  function createPuzzle(image) {
      const context = createContextFromImage(image);
      const piecesArray = [];
      
      // Define two empty positions (bottom right corner and one adjacent to it)
      const emptyPositions = [
          { x: pieces - 1, y: pieces - 1 },  // Bottom right
          { x: pieces - 2, y: pieces - 1 }   // One position to the left
      ];
      
      for (let y = 0; y < pieces; y++) {
          for (let x = 0; x < pieces; x++) {
              // Check if current position is one of the empty positions
              const isEmpty = emptyPositions.some(pos => pos.x === x && pos.y === y);
              
              if (isEmpty) {
                  piecesArray.push(null);
                  correctPieceOrder.push(null);
                  continue;
              }
              
              const canvas = document.createElement('canvas');
              canvas.width = pieceSize;
              canvas.height = pieceSize;
              const ctx = canvas.getContext('2d');
              ctx.putImageData(context.getImageData(x * pieceSize, y * pieceSize, pieceSize, pieceSize), 0, 0);
              canvas.classList.add('puzzle-piece');
              canvas.dataset.x = x;
              canvas.dataset.y = y;
              canvas.dataset.originalX = x;
              canvas.dataset.originalY = y;
              piecesArray.push(canvas);
              correctPieceOrder.push({ x, y });
          }
      }
      
      shuffle(piecesArray);
      renderPuzzle(piecesArray);
  }


  function createContextFromImage(image) {
      const canvas = document.createElement('canvas');
      canvas.width = qrSize;
      canvas.height = qrSize;
      const ctx = canvas.getContext('2d');
      ctx.drawImage(image, 0, 0, qrSize, qrSize);
      return ctx;
  }

  function shuffle(array) {
      for (let i = array.length - 1; i > 0; i--) {
          const j = Math.floor(Math.random() * (i + 1));
          [array[i], array[j]] = [array[j], array[i]];
      }
      document.getElementById('message').innerText = '!Solve It!';

  }

  function renderPuzzle(piecesArray) {
      puzzleContainer.innerHTML = '';
      piecesArray.forEach((piece, index) => {
          const x = (index % pieces) * pieceSize;
          const y = Math.floor(index / pieces) * pieceSize;
          
          if (piece) {
              piece.style.left = `${x}px`;
              piece.style.top = `${y}px`;
              piece.dataset.x = index % pieces;
              piece.dataset.y = Math.floor(index / pieces);
              puzzleContainer.appendChild(piece);
          } else {
              const emptyDiv = document.createElement('div');
              emptyDiv.classList.add('puzzle-piece', 'hidden');
              emptyDiv.style.left = `${x}px`;
              emptyDiv.style.top = `${y}px`;
              emptyDiv.dataset.x = index % pieces;
              emptyDiv.dataset.y = Math.floor(index / pieces);
              puzzleContainer.appendChild(emptyDiv);
          }
      });
  }

  // Update the handleMouseDown function for smoother dragging
  function findAdjacentEmptySpaces(piece) {
      const currentX = Math.floor(parseInt(piece.style.left) / pieceSize);
      const currentY = Math.floor(parseInt(piece.style.top) / pieceSize);
      const emptySpaces = Array.from(puzzleContainer.querySelectorAll('.hidden'));
      
      return emptySpaces.filter(emptySpace => {
          const emptyX = Math.floor(parseInt(emptySpace.style.left) / pieceSize);
          const emptyY = Math.floor(parseInt(emptySpace.style.top) / pieceSize);
          
          return (Math.abs(emptyX - currentX) === 1 && emptyY === currentY) ||
                (Math.abs(emptyY - currentY) === 1 && emptyX === currentX);
      });
  }

  function movePieceToSpace(piece, emptySpace) {
      const tempLeft = emptySpace.style.left;
      const tempTop = emptySpace.style.top;
      
      emptySpace.style.left = piece.style.left;
      emptySpace.style.top = piece.style.top;
      piece.style.left = tempLeft;
      piece.style.top = tempTop;
      
      const tempX = piece.dataset.x;
      const tempY = piece.dataset.y;
      piece.dataset.x = emptySpace.dataset.x;
      piece.dataset.y = emptySpace.dataset.y;
      
      checkSolved();
  }

  // Add these new functions for drag functionality
  function startDragging(e) {
      const piece = e.target;
      if (!piece.classList.contains('puzzle-piece') || piece.classList.contains('hidden')) return;

      isDragging = true;
      draggedPiece = piece;
      
      // Store the initial positions
      startX = e.clientX - piece.offsetLeft;
      startY = e.clientY - piece.offsetTop;
      originalX = parseInt(piece.style.left);
      originalY = parseInt(piece.style.top);
      
      // Add dragging class for visual feedback
      piece.style.zIndex = '1000';
      piece.style.opacity = '0.8';
  }

  function dragPiece(e) {
      if (!isDragging || !draggedPiece) return;
      e.preventDefault();

      const x = e.clientX - startX;
      const y = e.clientY - startY;
      
      // Limit movement to adjacent cells only
      const pieceX = Math.floor(originalX / pieceSize);
      const pieceY = Math.floor(originalY / pieceSize);
      
      draggedPiece.style.left = `${x}px`;
      draggedPiece.style.top = `${y}px`;
  }

  function stopDragging(e) {
      if (!isDragging || !draggedPiece) return;

      const finalX = parseInt(draggedPiece.style.left);
      const finalY = parseInt(draggedPiece.style.top);
      
      // Find the closest empty space
      const emptySpaces = Array.from(puzzleContainer.querySelectorAll('.hidden'));
      let closestSpace = null;
      let minDistance = Infinity;

      emptySpaces.forEach(space => {
          const spaceX = parseInt(space.style.left);
          const spaceY = parseInt(space.style.top);
          const distance = Math.abs(finalX - spaceX) + Math.abs(finalY - spaceY);
          
          if (distance < minDistance) {
              minDistance = distance;
              closestSpace = space;
          }
      });

      // Check if the move is valid (adjacent only)
      const isValidMove = isAdjacentMove(draggedPiece, closestSpace);
      
      if (isValidMove) {
          // Swap positions with the empty space
          const tempLeft = closestSpace.style.left;
          const tempTop = closestSpace.style.top;
          
          closestSpace.style.left = originalX + 'px';
          closestSpace.style.top = originalY + 'px';
          draggedPiece.style.left = tempLeft;
          draggedPiece.style.top = tempTop;
          
          // Update data attributes
          [draggedPiece.dataset.x, closestSpace.dataset.x] = [closestSpace.dataset.x, draggedPiece.dataset.x];
          [draggedPiece.dataset.y, closestSpace.dataset.y] = [closestSpace.dataset.y, draggedPiece.dataset.y];
          
          checkSolved();
      } else {
          // Return to original position if move is invalid
          draggedPiece.style.left = originalX + 'px';
          draggedPiece.style.top = originalY + 'px';
      }

      // Reset dragging states
      draggedPiece.style.zIndex = '';
      draggedPiece.style.opacity = '1';
      isDragging = false;
      draggedPiece = null;
  }

  function isAdjacentMove(piece, emptySpace) {
      const pieceX = parseInt(piece.dataset.x);
      const pieceY = parseInt(piece.dataset.y);
      const emptyX = parseInt(emptySpace.dataset.x);
      const emptyY = parseInt(emptySpace.dataset.y);

      return (Math.abs(emptyX - pieceX) === 1 && emptyY === pieceY) ||
            (Math.abs(emptyY - pieceY) === 1 && emptyX === pieceX);
  }

  // Update touch event handlers
  function handleTouchStart(e) {
      e.preventDefault();
      const touch = e.touches[0];
      const piece = e.target;
      
      if (!piece.classList.contains('puzzle-piece') || piece.classList.contains('hidden')) return;
      
      startDragging({
          target: piece,
          clientX: touch.clientX,
          clientY: touch.clientY,
          preventDefault: () => {}
      });
  }

  function handleTouchMove(e) {
      e.preventDefault();
      const touch = e.touches[0];
      
      dragPiece({
          clientX: touch.clientX,
          clientY: touch.clientY,
          preventDefault: () => {}
      });
  }

  function handleTouchEnd(e) {
      stopDragging(e);
  }

  function solvePuzzle() {
      const imageElement = new Image();
      imageElement.src = imageURL;
      imageElement.onload = () => {
          const context = createContextFromImage(imageElement);
          const piecesArray = [];
          const puzzleContainer = document.getElementById('puzzle-container');
          document.getElementById('message').innerText = 'Puzzle Solved!';


          for (let y = 0; y < pieces; y++) {
              for (let x = 0; x < pieces; x++) {
                  if (x === pieces - 1 && y === pieces - 1) {
                      emptyX = x;
                      emptyY = y;
                      piecesArray.push(null);
                      continue;
                  }
                  
                  const canvas = document.createElement('canvas');
                  canvas.width = pieceSize;
                  canvas.height = pieceSize;
                  const ctx = canvas.getContext('2d');
                  ctx.putImageData(context.getImageData(x * pieceSize, y * pieceSize, pieceSize, pieceSize), 0, 0);
                  canvas.classList.add('puzzle-piece');
                  canvas.dataset.x = x;
                  canvas.dataset.y = y;
                  piecesArray.push(canvas);
              }
          }

          // Arrange pieces according to the correctPieceOrder
          correctPieceOrder.forEach((position, index) => {
              if (position !== null) {
                  const { x, y } = position;
                  const piece = piecesArray.find(piece => parseInt(piece.dataset.x) === x && parseInt(piece.dataset.y) === y);
                  const correctX = index % 4;
                  const correctY = Math.floor(index / 4);
                  const targetX = correctX * pieceSize;
                  const targetY = correctY * pieceSize;

                  // Animate the transition of the piece to its target position
                  piece.style.transition = 'left 0.5s, top 0.5s';
                  piece.style.left = `${targetX}px`;
                  piece.style.top = `${targetY}px`;
                  piece.dataset.x = correctX;
                  piece.dataset.y = correctY;
              }
          });

          // Display the solved puzzle
          puzzleContainer.innerHTML = '';
          piecesArray.forEach(piece => puzzleContainer.appendChild(piece));
      };


  }

  // Update the movePiece function for better movement
  function movePiece(piece) {
      if (!piece || piece.classList.contains('hidden')) return false;

      const pieceX = parseInt(piece.dataset.x);
      const pieceY = parseInt(piece.dataset.y);
      
      const emptySpaces = Array.from(puzzleContainer.querySelectorAll('.hidden'));
      let moved = false;

      // Check all adjacent positions (up, down, left, right)
      emptySpaces.forEach(emptySpace => {
          const emptyX = parseInt(emptySpace.dataset.x);
          const emptyY = parseInt(emptySpace.dataset.y);

          // Check if the piece is adjacent horizontally or vertically
          const isAdjacent = (
              // Horizontal adjacent (left or right)
              (Math.abs(emptyX - pieceX) === 1 && emptyY === pieceY) ||
              // Vertical adjacent (up or down)
              (Math.abs(emptyY - pieceY) === 1 && emptyX === pieceX)
          );

          if (isAdjacent && !moved) {
              // Animate the movement
              piece.style.transition = 'left 0.2s, top 0.2s';
              emptySpace.style.transition = 'left 0.2s, top 0.2s';
              
              // Swap positions
              const tempLeft = piece.style.left;
              const tempTop = piece.style.top;
              
              piece.style.left = emptySpace.style.left;
              piece.style.top = emptySpace.style.top;
              emptySpace.style.left = tempLeft;
              emptySpace.style.top = tempTop;
              
              // Update data attributes
              [piece.dataset.x, emptySpace.dataset.x] = [emptySpace.dataset.x, piece.dataset.x];
              [piece.dataset.y, emptySpace.dataset.y] = [emptySpace.dataset.y, piece.dataset.y];
              
              moved = true;
          }
      });

      if (moved) {
          checkSolved();
          return true;
      }
      return false;
  }

  // Add this function after the existing checkSolved function
  function showCompleteImage() {
      const hiddenPieces = puzzleContainer.querySelectorAll('.hidden');
      hiddenPieces.forEach(piece => {
          // Create new piece div
          const newPiece = document.createElement('div');
          newPiece.classList.add('puzzle-piece');
          
          // Set position
          newPiece.style.left = piece.style.left;
          newPiece.style.top = piece.style.top;
          
          // Calculate background position
          const x = parseInt(piece.dataset.x) * pieceSize;
          const y = parseInt(piece.dataset.y) * pieceSize;
          
          // Set background image and position
          newPiece.style.backgroundImage = `url(${imageURL})`;
          newPiece.style.backgroundPosition = `-${x}px -${y}px`;
          newPiece.style.backgroundSize = `${qrSize}px ${qrSize}px`;
          
          // Add fade-in animation
          newPiece.style.animation = 'fadeIn 0.5s ease-in';
          
          // Replace hidden piece with new piece
          piece.replaceWith(newPiece);
      });
  }

  // Add this CSS animation
  const style = document.createElement('style');
  style.textContent = `
      @keyframes fadeIn {
          from { opacity: 0; }
          to { opacity: 1; }
      }
  `;
  document.head.appendChild(style);

  // Modify the checkSolved function to call showCompleteImage when solved
  function checkSolved() {
      let solved = true;
      const pieces = puzzleContainer.querySelectorAll('.puzzle-piece:not(.hidden)');
      
      pieces.forEach((piece) => {
          const currentX = Math.floor(parseInt(piece.style.left) / pieceSize);
          const currentY = Math.floor(parseInt(piece.style.top) / pieceSize);
          const originalX = parseInt(piece.dataset.originalX);
          const originalY = parseInt(piece.dataset.originalY);
          
          if (currentX !== originalX || currentY !== originalY) {
              solved = false;
          }
      });

      document.getElementById('message').innerText = solved ? 'Puzzle Solved!' : '!Solve It!';
      
      if (solved) {
          showCompleteImage(); // Add this line to show the complete image
          fetch('/puzzleSolved', {method: 'POST'})
              .then(response => console.log('Notified ESP32'))
              .catch(error => console.error('Error:', error));

          fetch('/led/on')
              .then(response => response.text())
              .then(data => console.log(data))
              .catch(error => console.error('Error:', error));
      
      }
  }

  // Update the shufflePuzzle function for better shuffling
  function shufflePuzzle() {
      const pieces = Array.from(puzzleContainer.querySelectorAll('.puzzle-piece'));
      
      // Perform many random valid moves to ensure a good shuffle
      for (let i = 0; i < 100; i++) {
          const emptySpace = puzzleContainer.querySelector('.hidden');
          const emptyX = parseInt(emptySpace.dataset.x);
          const emptyY = parseInt(emptySpace.dataset.y);
          
          // Find all movable pieces
          const movablePieces = pieces.filter(piece => {
              if (piece.classList.contains('hidden')) return false;
              const pieceX = parseInt(piece.dataset.x);
              const pieceY = parseInt(piece.dataset.y);
              return (Math.abs(emptyX - pieceX) === 1 && emptyY === pieceY) ||
                    (Math.abs(emptyY - pieceY) === 1 && emptyX === pieceX);
          });
          
          if (movablePieces.length > 0) {
              const randomPiece = movablePieces[Math.floor(Math.random() * movablePieces.length)];
              movePiece(randomPiece);
          }
      }
      
      document.getElementById('message').innerText = '!Solve It!';
  }





  )rawliteral";

void handlePuzzleSolved(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", "OK");
}

void setupServer()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
          String html = String(index_html);
          html.replace("</head>", "<style>" + String(styles_css) + "</style></head>");
          html.replace("</body>", "<script>" + String(script_js) + "</script></body>");
          request->send(200, "text/html", html); });

    server.on("/puzzleSolved", HTTP_POST, [](AsyncWebServerRequest *request)
              { handlePuzzleSolved(request); });

    server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request)
              {
  //digitalWrite(LED_PIN, HIGH); // Turn on the LED
  isSolved =1;
  request->send(200, "text/plain", "TASK 7 completed! "); });

    server.begin();
    Serial.println("HTTP server started for puzzle game");
}
