class SiteHeader extends HTMLElement {
  constructor() {
    super();
  }

  connectedCallback() {
    this.innerHTML = `
      <header>
        <div class="nav-container">
          <nav class="navbar no-transition">
            <div class="nav-links">
              <a href="index.html" class="nav-link">Home</a>
              <a href="team.html" class="nav-link">Team</a>
              <a href="problem.html" class="nav-link">Problem</a>
              <a href="solution.html" class="nav-link">Solution</a>
              <a href="testing.html" class="nav-link">Testing</a>
              <a href="timeline.html" class="nav-link">Timeline/Milestones</a>
              <a href="media.html" class="nav-link">Media</a>
              <a href="tutorials.html" class="nav-link">Tutorials/References</a>
              <a href="updates.html" class="nav-link">Video/Poster Updates</a>
            </div>
          </nav>
        </div>
        <div class="viewport-warning">
          <div class="warning-content">
            <h2>Widen your browser to view.</h2>
          </div>
        </div>
      </header>
    `;

    this.highlightCurrentPage();
    this.setupScrollHandler();
  }

  highlightCurrentPage() {
    const path = window.location.pathname;
    const page = path.split("/").pop();

    const navLinks = this.querySelectorAll(".nav-link");
    navLinks.forEach((link) => {
      const href = link.getAttribute("href");

      if (href === "index.html") {
        if (page === "" || page === "index.html") {
          link.classList.add("active");
        }
      } else if (href === page) {
        link.classList.add("active");
      }
    });
  }

  setupScrollHandler() {
    const navbar = document.querySelector(".navbar");

    if (window.scrollY <= 0) {
      navbar.classList.add("at-top");
    }

    setTimeout(() => {
      navbar.classList.remove("no-transition");
    }, 100);

    window.addEventListener("scroll", () => {
      if (window.scrollY <= 0) {
        navbar.classList.add("at-top");
      } else {
        navbar.classList.remove("at-top");
      }
    });
  }
}

customElements.define("site-header", SiteHeader);
