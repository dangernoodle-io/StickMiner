<section class="card">
  <h2>Configuration Saved</h2>
  <div class="spinner"></div>
  <p>Connecting to WiFi...</p>
</section>

<style>
  .card {
    background: var(--surface);
    border: 1px solid var(--input);
    border-radius: 8px;
    padding: 30px;
    text-align: center;
  }
  h2 {
    color: var(--accent);
    margin: 0 0 1rem;
  }
  .spinner {
    display: inline-block;
    width: 30px;
    height: 30px;
    border: 3px solid var(--input);
    border-top: 3px solid var(--accent);
    border-radius: 50%;
    animation: spin 1s linear infinite;
  }
  p {
    margin-top: 20px;
    color: var(--label);
  }
  @keyframes spin {
    0% { transform: rotate(0deg); }
    100% { transform: rotate(360deg); }
  }
</style>
