document.addEventListener("DOMContentLoaded", () => {
  const carouselImageSets = {
    carouselBuildsDesigns: [
      {
        src: "./images/pcbanno.png",
        caption: "PCB layout",
      },
      {
        src: "./images/topfinalpcb.png",
        caption: "Final assembled board (top)",
      },
      {
        src: "./images/botfinalpcb.png",
        caption: "Final assembled board (bottom)",
      },
      {
        src: "./images/onshapecad.png",
        caption: "3D print CAD model",
      },      
      {
        src: "./images/3dprintbutton.jpg",
        caption: "3D print showing how buttons work",
      },
      {
        src: "./images/finalbuild.jpg",
        caption: "Picture of final build",
      },
      {
        src: "./images/screen3.jpg",
        caption: "Picture of overall report screen",
      },
      // Add more images as needed
    ],

    carouselSchematicsDiagrams: [
      {
        src: "./images/block.png",
        caption: "System Architecture",
      },
      {
        src: "./images/state.png",
        caption: "Simple State Diagram",
      },
      {
        src: "./images/schematicanno.png",
        caption: "Annotated Schematic",
      },
      // Add more images as needed
    ],
    
    carouselPcbDesign: [
      {
        src: "./images/pcbanno.png",
        caption: "PCB layout",
      },
      {
        src: "./images/schematicanno.png",
        caption: "Annotated Schematic",
      },
      {
        src: "./images/3dviewer.png",
        caption: "Top side of PCB in 3D viewer",
      },
      {
        src: "./images/3dviewer1.png",
        caption: "Bottom side of PCB in 3D viewer",
      },
      {
        src: "./images/stencil.jpg",
        caption: "Stencil and PCB",
      },
      {
        src: "./images/topfinalpcb.png",
        caption: "Final assembled board (top)",
      },
      {
        src: "./images/botfinalpcb.png",
        caption: "Final assembled board (bottom)",
      },
    ],
    
    carouselUpdate1: [
      {
        src: "./images/pinbudgetsheet.png",
        caption: "Pin budget section of Google sheet",
      },
      {
        src: "./images/featherfootprint.png",
        caption: "Custom Adafruit Feather V2 Footprint",
      },
      {
        src: "./images/feathersymbol.png",
        caption: "Custom Adafruit Feather V2 Symbol",
      },
      {
        src: "./images/ldoschematic.png",
        caption: "LDO regulator setup for extra rail",
      },
    ],
    
    carouselUpdate2: [
      {
        src: "./images/displaytesting.png",
        caption: "Testing the display",
      },
      {
        src: "./images/breakouttesting.png",
        caption: "Code for testing the breakouts",
      },
    ],
    
    carouselUpdate3: [
      {
        src: "./images/soldering.jpg",
        caption: "Soldering headers onto PCB",
      },
      {
        src: "./images/onshapecad.png",
        caption: "3D print CAD model",
      },
      {
        src: "./images/3dprinting.jpg",
        caption: "3D printing",
      },
      {
        src: "./images/3dprintempty.jpg",
        caption: "3D print skeleton",
      },
      {
        src: "./images/3dprintpcb.jpg",
        caption: "3D print with PCB",
      },
      {
        src: "./images/3dprintbutton.jpg",
        caption: "3D print showing how buttons work",
      },
      {
        src: "./images/displaycode.png",
        caption: "Code for displaying data",
      },
      {
        src: "./images/initialprototype.jpg",
        caption: "Fully working prototype",
      },
    ],
    
    carouselUpdate4: [
      {
        src: "./images/finalbuild.jpg",
        caption: "Final device with Holistic Readings",
      },
      {
        src: "./images/refresh.png",
        caption: "Refreshing Display Every 10 Seconds",
      },
      {
        src: "./images/debounce.png",
        caption: "Fix for False Button Presses",
      },
      {
        src: "./images/screen3.jpg",
        caption: "Overall Report",
      },
      {
        src: "./images/screen2.jpg",
        caption: "More Granular Readings",
      },
      {
        src: "./images/screen1.jpg",
        caption: "CO2 + Altitude Readings with Tooltips",
      },
    ],
    
    carouselTutorials: [
      {
        src: "./images/evantut.jpg",
        caption: "Evan's Tutorial (OLED Display)",
      },
      {
        src: "./images/marktut.png",
        caption: "Mark's Tutorial (DHT11)",
      },
      {
        src: "./images/kevintut.jpeg",
        caption: "Kevin's Tutorial (DHT11)",
      },
    ],
    
    carouselTesting1: [
      {
        src: "./images/tempchart.png",
        caption: "Temperature and Humidity Test Results",
      },
      {
        src: "./images/co2chart.png",
        caption: "CO2 Test Results",
      },
      {
        src: "./images/dBchart.png",
        caption: "Sound Intensity Test Results",
      },
    ],
    
    carouselTesting2: [
      {
        src: "./images/db6.jpg",
        caption: "Manual Decibel Test",
      },
      {
        src: "./images/temp1.jpg",
        caption: "Temperature Test 1",
      },
      {
        src: "./images/temp2.jpg",
        caption: "Temperature Test 2",
      },
      {
        src: "./images/temp3.jpg",
        caption: "Temperature Test 3",
      },
      {
        src: "./images/temp4.jpg",
        caption: "Temperature Test 4",
      },
      {
        src: "./images/temp5.jpg",
        caption: "Temperature Test 5",
      },
      {
        src: "./images/co21.jpg",
        caption: "CO2 Test 1",
      },
      {
        src: "./images/co22.jpg",
        caption: "CO2 Test 2",
      },
      {
        src: "./images/co23.jpg",
        caption: "CO2 Test 3",
      },
      {
        src: "./images/co24.jpg",
        caption: "CO2 Test 4",
      },
      {
        src: "./images/co25.jpg",
        caption: "CO2 Test 5",
      },
      {
        src: "./images/db1.jpg",
        caption: "Decibel Test 1",
      },
      {
        src: "./images/db2.jpg",
        caption: "Decibel Test 2",
      },
      {
        src: "./images/db3.jpg",
        caption: "Decibel Test 3",
      },
      {
        src: "./images/db4.jpg",
        caption: "Decibel Test 4",
      },
      {
        src: "./images/db5.jpg",
        caption: "Decibel Test 5",
      },
    ],
  };

  function initializeCarousel(carouselId) {
    const carouselImagesContainer = document.getElementById(
      `${carouselId}-images`
    );
    
    // Check if the carousel exists on this page
    if (!carouselImagesContainer) {
      console.log(`Carousel ${carouselId} not found on this page, skipping initialization.`);
      return null;
    }
    
    const carouselImages = carouselImageSets[carouselId];
    // Construct the full ID for caption, prev, and next buttons
    const captionId = `caption${carouselId.substring('carousel'.length)}`;
    const prevButtonId = `prev${carouselId.substring('carousel'.length)}`;
    const nextButtonId = `next${carouselId.substring('carousel'.length)}`;

    const carouselCaption = document.getElementById(captionId);
    const prevButton = document.getElementById(prevButtonId);
    const nextButton = document.getElementById(nextButtonId);

    let currentImageIndex = 0;

    function initCarousel() {
      carouselImages.forEach((image, index) => {
        const imgElement = document.createElement("img");
        imgElement.src = image.src;
        imgElement.alt = image.caption;
        imgElement.classList.add("carousel-image");

        if (index === 0) {
          imgElement.classList.add("active");
          carouselCaption.textContent = image.caption;
        }

        carouselImagesContainer.appendChild(imgElement);
      });
    }

    function showImage(index) {
      if (index < 0) index = carouselImages.length - 1;
      if (index >= carouselImages.length) index = 0;

      const images =
        carouselImagesContainer.querySelectorAll(".carousel-image");

      images[currentImageIndex].classList.remove("active");

      currentImageIndex = index;
      images[currentImageIndex].classList.add("active");

      carouselCaption.textContent = carouselImages[currentImageIndex].caption;
    }

    prevButton.addEventListener("click", () => {
      showImage(currentImageIndex - 1);
    });

    nextButton.addEventListener("click", () => {
      showImage(currentImageIndex + 1);
    });

    initCarousel();

    return {
      showImage,
      getCurrentIndex: () => currentImageIndex,
    };
  }

  // Only initialize carousels that exist on the current page
  if (document.getElementById("carouselBuildsDesigns-images")) {
    initializeCarousel("carouselBuildsDesigns");
  }
  
  if (document.getElementById("carouselSchematicsDiagrams-images")) {
    initializeCarousel("carouselSchematicsDiagrams");
  }
  
  if (document.getElementById("carouselPcbDesign-images")) {
    initializeCarousel("carouselPcbDesign");
  }
  
  if (document.getElementById("carouselUpdate1-images")) {
    initializeCarousel("carouselUpdate1");
  }
  
  if (document.getElementById("carouselUpdate2-images")) {
    initializeCarousel("carouselUpdate2");
  }
  
  if (document.getElementById("carouselUpdate3-images")) {
    initializeCarousel("carouselUpdate3");
  }
  
  if (document.getElementById("carouselUpdate4-images")) {
    initializeCarousel("carouselUpdate4");
  }
  
  if (document.getElementById("carouselTutorials-images")) {
    initializeCarousel("carouselTutorials");
  }
  
  if (document.getElementById("carouselTesting1-images")) {
    initializeCarousel("carouselTesting1");
  }
  
  if (document.getElementById("carouselTesting2-images")) {
    initializeCarousel("carouselTesting2");
  }
});
