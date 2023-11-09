# build environment
FROM node:14.5.0-alpine 

## ENV variables
ENV PERM=active

WORKDIR /app
ENV PATH /app/node_modules/.bin:$PATH
COPY scripts/package.json ./
COPY scripts/package-lock.json ./
COPY scripts/docker-updater.js ./updater.js
COPY scripts/docker.env .env


RUN npm ci --silent
RUN npm install react-scripts@3.4.1 -g --silent


# Entrypoint
ADD scripts/start.sh /
RUN chmod +x /start.sh
CMD /start.sh
