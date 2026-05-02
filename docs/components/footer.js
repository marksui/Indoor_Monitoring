class SiteFooter extends HTMLElement {
  constructor() {
    super();
  }

  connectedCallback() {
    this.innerHTML = `
      <footer>
        <div class="footer-container">
          <img src="./images/ecelogo.png" alt="ECE Logo" class="footer-logo">
        </div>
      </footer>
    `;
  }
}

customElements.define("site-footer", SiteFooter);
