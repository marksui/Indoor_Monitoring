// Wait for DOM to be fully loaded
document.addEventListener("DOMContentLoaded", () => {
  const animatedElements = document.querySelectorAll(".animate__animated");

  const observer = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        if (entry.isIntersecting) {
          let hasAnimationClass = false;

          entry.target.classList.forEach((cls) => {
            if (cls.startsWith("animate__") && cls !== "animate__animated") {
              hasAnimationClass = true;
            }
          });

          if (!hasAnimationClass) {
            const animation = entry.target.dataset.animation || "fadeIn";
            entry.target.classList.add(`animate__${animation}`);
          }

          observer.unobserve(entry.target);
        }
      });
    },
    { threshold: 0.1 }
  );

  animatedElements.forEach((el) => observer.observe(el));
});
