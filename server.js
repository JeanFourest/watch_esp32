const express = require('express');
const app = express();
const port = 5000;

let lastNotification = { app: 'Aucune', message: 'Aucune notification' };

app.use(express.json());

app.post('/notification', (req, res) => {
  const { message, app } = req.body;
  if (!message && !app) {
    return res.status(400).json({ status: 'erreur', message: 'Données manquantes' });
  }
  lastNotification = { app, message };
  console.log(`🔔 [${app}] ${message}`);
  res.status(200).json();
});

app.get('/notification', (req, res) => {
  res.json(lastNotification);
});

app.listen(port, () => {
  console.log(`🚀 Serveur actif sur http://localhost:${port}`);
});